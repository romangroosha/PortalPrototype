#include "PortalAudit.h"
#include "PortalLab.h"
#include "PortalMath.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "RHI.h"
#include "UnrealClient.h"

namespace {
struct FCase { const TCHAR* Name; FVector Eye; FRotator View; float FOV; };
const FCase Cases[] = {
 {TEXT("center"), FVector(0,0,154.15), FRotator(0,0,0), 90},
 {TEXT("oblique"), FVector(360,180,154.15), FRotator(-3,-37,0), 90},
 {TEXT("near"), FVector(595,20,154.15), FRotator(0,0,0), 90},
 {TEXT("subnear"), FVector(599.5,0,154.15), FRotator(0,0,0), 90},
 {TEXT("on_plane"), FVector(600,0,154.15), FRotator(0,0,0), 90},
 {TEXT("pitch"), FVector(480,20,210), FRotator(-20,-10,0), 90},
 {TEXT("wide"), FVector(300,-90,154.15), FRotator(0,15,0), 110},
};
const TCHAR* Benchmarks[] = {TEXT("frozen_capture"), TEXT("live_capture"), TEXT("moving_view"), TEXT("traversal"), TEXT("traversal_60fps"), TEXT("fast_traversal_30fps")};
constexpr int VisualStages = UE_ARRAY_COUNT(Cases) * 2;
}
APortalAudit::APortalAudit() {
 PrimaryActorTick.bCanEverTick = true;
 PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}
void APortalAudit::BeginPlay() {
 Super::BeginPlay();
 Directory = FPaths::ProjectSavedDir() / TEXT("PortalAudit");
 IFileManager::Get().MakeDirectory(*Directory, true);
 Pawn = Cast<APortalPawn>(UGameplayStatics::GetPlayerPawn(this,0));
 check(Pawn);
 Pawn->GetCharacterMovement()->DisableMovement();
 Camera = GetWorld()->SpawnActor<ACameraActor>();
 Camera->GetCameraComponent()->PostProcessSettings = Pawn->Camera->PostProcessSettings;
 Camera->GetCameraComponent()->bConstrainAspectRatio = false;
 UGameplayStatics::GetPlayerController(this,0)->SetViewTarget(Camera);
 for (TActorIterator<APortalGate> It(GetWorld()); It; ++It) {
  Gates.Add(*It);
  AddTickPrerequisiteActor(*It);
  if (It->GetActorLocation().X > 0) Entry = *It;
 }
 check(Entry && Entry->Linked);
 for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
  if (FMath::Abs(It->GetActorLocation().X - Entry->Linked->GetActorLocation().X) < 20) ExitWall.Add(*It);
 Stage = 0;
 SetVisualCase(Stage);
}
void APortalAudit::SetVisualCase(int32 Index) {
 const FCase& C = Cases[Index / 2];
 const bool bReference = (Index % 2) != 0;
 for (auto Gate : Gates) if (Gate.IsValid()) {
  Gate->Surface->SetVisibility(!bReference || Gate.Get()==Entry);
  Gate->RecursionDepth = bReference ? 2 : 3;
 }
 for (auto Wall : ExitWall) if (Wall.IsValid()) Wall->SetActorHiddenInGame(bReference);
 // Independent reference: real player viewport at the analytically mapped eye, no capture texture.
 const FTransform From = Entry->GetActorTransform(), To = Entry->Linked->GetActorTransform();
 const FVector Eye = bReference ? PortalMath::Point(C.Eye, From, To) : C.Eye;
 const FQuat Rotation = bReference ? PortalMath::Rotation(From, To) * C.View.Quaternion() : C.View.Quaternion();
 Camera->SetActorLocationAndRotation(Eye, Rotation);
 Camera->GetCameraComponent()->FieldOfView = C.FOV;
 Frames = 0; bScreenshot = false;
 StartTime = FPlatformTime::Seconds();
}
void APortalAudit::SetBenchmark(int32 Index) {
 for (auto Gate : Gates) if (Gate.IsValid()) {
  Gate->Surface->SetVisibility(true);
  Gate->RecursionDepth = 3;
  Gate->SetActorTickEnabled(Index != 0);
 }
 for (auto Wall : ExitWall) if (Wall.IsValid()) Wall->SetActorHiddenInGame(false);
 Camera->SetActorLocationAndRotation(FVector(350,30,154.15), FRotator(0,0,0));
 Camera->GetCameraComponent()->FieldOfView = 90;
 if (Index >= 3) {
  Pawn->SetActorLocation(FVector(0,0,90.15), false, nullptr, ETeleportType::TeleportPhysics);
  Pawn->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
  Pawn->GetCharacterMovement()->StopMovementImmediately();
  Pawn->GetCharacterMovement()->MaxWalkSpeed = Index==5 ? 2400 : 600;
  auto* PC = UGameplayStatics::GetPlayerController(this,0);
  PC->SetControlRotation(FRotator::ZeroRotator);
  PC->SetViewTarget(Pawn);
  PC->ConsoleCommand(Index==4 ? TEXT("t.MaxFPS 60") : Index==5 ? TEXT("t.MaxFPS 30") : TEXT("t.MaxFPS 0"), false);
  PreviousPawnPosition = Pawn->GetActorLocation();
 }
 FrameTimes.Reset(); Frames = 0;
 StartTime = PreviousTime = FPlatformTime::Seconds();
 UE_LOG(LogTemp, Display, TEXT("AUDIT_BENCH_BEGIN %s"), Benchmarks[Index]);
 int32 W,H; UGameplayStatics::GetPlayerController(this,0)->GetViewportSize(W,H);
 UE_LOG(LogTemp, Display, TEXT("AUDIT_VIEWPORT %dx%d"),W,H);
}
void APortalAudit::FinishBenchmark() {
 TArray<double> Sorted = FrameTimes; Sorted.Sort();
 double Total = 0;
 for (double V : Sorted) Total += V;
 if (!Sorted.IsEmpty()) {
  UE_LOG(LogTemp, Display, TEXT("AUDIT_BENCH %s frames=%d mean_ms=%.3f fps=%.1f p95_ms=%.3f p99_ms=%.3f max_ms=%.3f"),
   Benchmarks[Stage-VisualStages], Sorted.Num(), Total/Sorted.Num(), 1000*Sorted.Num()/Total,
   Sorted[FMath::FloorToInt((Sorted.Num()-1)*.95)], Sorted[FMath::FloorToInt((Sorted.Num()-1)*.99)], Sorted.Last());
 }
}
void APortalAudit::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);
 if (Stage < 0) return;
 ++Frames;
 const double Now = FPlatformTime::Seconds();
 const double Elapsed = Now - StartTime;
 auto* PC = UGameplayStatics::GetPlayerController(this,0);
 if (Stage < VisualStages) {
  if (!bScreenshot && Frames > 20 && Elapsed > .7) {
   const FCase& C = Cases[Stage / 2];
   const FString Name = FString(C.Name) + (Stage % 2 ? TEXT("_reference") : TEXT("_portal"));
   FScreenshotRequest::RequestScreenshot(Directory / (Name + TEXT(".png")), false, false);
   if (Stage % 2 == 0) {
    FString Polygon;
    for (FVector Local : {FVector(0,-90,-140), FVector(0,90,-140), FVector(0,90,140), FVector(0,-90,140)}) {
     FVector2D Screen;
     PC->ProjectWorldLocationToScreen(Entry->GetActorTransform().TransformPosition(Local),Screen);
     Polygon += FString::Printf(TEXT("%.6f,%.6f\n"),Screen.X,Screen.Y);
    }
    if (FString(C.Name) == TEXT("on_plane")) {
     // Centered eye on the mathematical plane: aperture covers the entire viewport,
     // but its zero-depth corners cannot be projected to finite screen coordinates.
     int32 W,H; PC->GetViewportSize(W,H);
     Polygon = FString::Printf(TEXT("0,0\n%d,0\n%d,%d\n0,%d\n"),W,W,H,H);
    }
    FFileHelper::SaveStringToFile(Polygon, *(Directory / (FString(C.Name)+TEXT("_polygon.csv"))));
   }
   bScreenshot = true;
   UE_LOG(LogTemp, Display, TEXT("AUDIT_IMAGE %s"), *Name);
  }
  if (bScreenshot && Elapsed > 1.1) {
   ++Stage;
   if (Stage < VisualStages) SetVisualCase(Stage);
   else SetBenchmark(0);
  }
  return;
 }
 const int32 Benchmark = Stage - VisualStages;
 const double FrameMS = (Now - PreviousTime)*1000;
 PreviousTime = Now;
 if (Benchmark == 2) {
  Camera->SetActorLocationAndRotation(FVector(420, 70*FMath::Sin(Elapsed*1.7),154.15+25*FMath::Sin(Elapsed)),
   FRotator(5*FMath::Sin(Elapsed), 15*FMath::Sin(Elapsed*1.7),0));
 }
 if (Benchmark >= 3) Pawn->AddMovementInput(FVector::ForwardVector,1);
 double CameraError = 0;
 bool bCrossed = false;
 if (Benchmark >= 3) {
  FVector Eye; FRotator View; PC->GetPlayerViewPoint(Eye,View);
  CameraError = FVector::Distance(Eye,Pawn->Camera->GetComponentLocation());
  MaxCameraError = FMath::Max(MaxCameraError,CameraError);
  bCrossed = Pawn->GetActorLocation().X-PreviousPawnPosition.X < -1000;
  Traversals += bCrossed ? 1 : 0;
  PreviousPawnPosition = Pawn->GetActorLocation();
 }
 int32 Captures = 0;
 for (auto Gate : Gates) if (Gate.IsValid() && Gate->LastCaptureFrame == GFrameCounter) Captures += Gate->LastRenderPasses;
 // Warmup and all screenshot readbacks excluded from benchmark samples.
 if (Elapsed > 2) {
  FrameTimes.Add(FrameMS);
  const double GPUMS = FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
  SamplesCSV += FString::Printf(TEXT("%s,%.6f,%.6f,%.6f,%d,%d\n"),Benchmarks[Benchmark],FrameMS,GPUMS,CameraError,Captures,bCrossed?1:0);
 }
 if (Elapsed > 12) {
  FinishBenchmark();
  ++Stage;
  if (Stage-VisualStages < UE_ARRAY_COUNT(Benchmarks)) SetBenchmark(Stage-VisualStages);
  else {
   FFileHelper::SaveStringToFile(SamplesCSV, *(Directory / TEXT("frames.csv")));
   UE_LOG(LogTemp, Display, TEXT("PORTAL_AUDIT_COMPLETE"));
   UE_LOG(LogTemp, Display, TEXT("AUDIT_CAMERA max_error_cm=%.6f traversals=%d"),MaxCameraError,Traversals);
   Stage = -1;
   FPlatformMisc::RequestExit(false);
  }
 }
}
