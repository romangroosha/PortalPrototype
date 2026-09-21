#include "PortalTraversalAudit.h"
#include "PortalLab.h"
#include "PortalGun.h"
#include "PortalMath.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"

APortalTraversalAudit::APortalTraversalAudit() { PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.TickGroup=TG_PostUpdateWork; }
void APortalTraversalAudit::Check(bool bValue,const TCHAR* Name) {
 ++Checks; if(!bValue) ++Failures;
 UE_LOG(LogTemp,Display,TEXT("TRAVERSAL_CHECK %s: %s"),bValue?TEXT("PASS"):TEXT("FAIL"),Name);
}
void APortalTraversalAudit::Tick(float Dt) {
 Super::Tick(Dt); Time+=Dt;
 auto* P=Cast<APortalPawn>(UGameplayStatics::GetPlayerPawn(this,0));
 if(!P) return;
 if(FParse::Param(FCommandLine::Get(),TEXT("PortalTraversalPreview"))) {
  if(Stage==0&&Time>2) {
   APortalWall* Floor=nullptr; APortalWall* East=nullptr;
   for(TActorIterator<APortalWall> It(GetWorld());It;++It) {
    UE_LOG(LogTemp,Display,TEXT("TRAVERSAL_PANEL %s location=%s rotation=%s width=%.1f height=%.1f bounds=%s"),*It->GetName(),*It->GetActorLocation().ToString(),*It->GetActorRotation().ToString(),It->HalfWidth,It->HalfHeight,*It->GetComponentsBoundingBox(true).GetSize().ToString());
    if(It->ActorHasTag(TEXT("TraversalFloor"))) Floor=*It;
    if(It->ActorHasTag(TEXT("TraversalEast"))) East=*It;
   }
   Check(Floor&&East,TEXT("Authored traversal map contains oriented surfaces"));
   if(Floor&&East) {
    Check(P->Gun->FireRay(false,FVector(200,200,300),FVector(0,0,-1)),TEXT("Authored floor accepts portal"));
    Check(P->Gun->FireRay(true,FVector(1300,200,550),FVector(1,0,0)),TEXT("Authored wall accepts linked portal"));
    P->GetController()->SetControlRotation(FRotator(-8,15,0));
   }
   Stage=1; Time=0;
  } else if(Stage==1&&Time>1) {
   Check(FMath::Abs(FRotator::NormalizeAxis(P->Camera->GetComponentRotation().Roll))<.05,TEXT("Camera manager preserves upright roll across real frames"));
   auto* Gate=P->Gun->GetPortal(0);
   if(Gate&&Gate->Capture->TextureTarget) {
    auto* RT=Gate->Capture->TextureTarget.Get(); TArray<FColor> Pixels; TArray64<uint8> PNG;
    RT->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
    FVector2D Screen; auto* PC=Cast<APlayerController>(P->GetController()); int32 W=0,H=0; PC->GetViewportSize(W,H);
    const bool bProjected=PC->ProjectWorldLocationToScreen(Gate->GetActorLocation(),Screen);
    const int32 X=FMath::Clamp(FMath::RoundToInt(Screen.X*RT->SizeX/FMath::Max(1,W)),0,RT->SizeX-1);
    const int32 Y=FMath::Clamp(FMath::RoundToInt(Screen.Y*RT->SizeY/FMath::Max(1,H)),0,RT->SizeY-1);
    const FColor Center=Pixels.IsValidIndex(Y*RT->SizeX+X)?Pixels[Y*RT->SizeX+X]:FColor::Black;
    Check(bProjected&&int32(Center.R)+Center.G+Center.B>60,TEXT("Distant floor portal renders lit destination at aperture center"));
    FImageUtils::PNGCompressImageArray(RT->SizeX,RT->SizeY,Pixels,PNG);
    FFileHelper::SaveArrayToFile(PNG,*(FPaths::ProjectSavedDir()/TEXT("TraversalCapture.png")));
    UE_LOG(LogTemp,Display,TEXT("TRAVERSAL_VIEW player=%s view=%s capture=%s rotation=%s passes=%d"),*P->Camera->GetComponentLocation().ToString(),*P->Camera->GetComponentRotation().ToString(),*Gate->Capture->GetComponentLocation().ToString(),*Gate->Capture->GetComponentRotation().ToString(),Gate->LastRenderPasses);
   }
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("TraversalLab.png"),false,false); Stage=2; Time=0;
  } else if(Stage==2&&Time>.5f) {
   const FVector Offsets[]={FVector(-600,0,200),FVector(-200,0,300),FVector(0,0,350),FVector(-300,0,20),FVector(0,0,5)};
   if(ViewCase>=UE_ARRAY_COUNT(Offsets)) {
    UE_LOG(LogTemp,Display,TEXT("PORTAL_FLOOR_VIEW_AUDIT %s checks=%d failures=%d"),Failures?TEXT("FAIL"):TEXT("PASS"),Checks,Failures);
    FPlatformMisc::RequestExitWithStatus(false,Failures?1:0); Stage=4;
   } else {
    auto* Gate=P->Gun->GetPortal(0);
    const FVector Eye=Gate->GetActorLocation()+Offsets[ViewCase];
    P->GetCharacterMovement()->DisableMovement();
    P->SetActorEnableCollision(false);
    P->SetActorLocation(Eye-FVector(0,0,64),false,nullptr,ETeleportType::TeleportPhysics);
    P->GetController()->SetControlRotation((-Offsets[ViewCase]).Rotation());
    P->ResetPortalTracking(true); Stage=3; Time=0;
   }
  } else if(Stage==3&&Time>.5f) {
   auto* Gate=P->Gun->GetPortal(0);
   Check(Gate->LastCaptureFrame+2>=GFrameCounter,TEXT("Floor view stays live at distant, overhead, grazing and near-plane poses"));
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("FloorView%d.png"),ViewCase),false,false);
   ++ViewCase; Stage=2; Time=0;
  }
  return;
 }
 if(Stage==0&&Time>2) {
  struct FCase { FRotator From,To; const TCHAR* Name; };
  const FCase Cases[]={
   {FRotator(90,0,0),FRotator(0,25,0),TEXT("floor to wall")},
   {FRotator(0,25,0),FRotator(90,0,0),TEXT("wall to floor")},
   {FRotator(-90,0,0),FRotator(0,45,0),TEXT("ceiling to wall")},
   {FRotator(0,45,0),FRotator(-90,0,0),TEXT("wall to ceiling")},
   {FRotator(45,30,20),FRotator(-35,110,40),TEXT("inclined surfaces")},
   {FRotator(90,0,0),FRotator(90,90,0),TEXT("floor to floor")}
  };
  const FVector Base(20000,20000,10000);
  auto* A=GetWorld()->SpawnActor<APortalWall>(Base,FRotator::ZeroRotator);
  auto* B=GetWorld()->SpawnActor<APortalWall>(Base+FVector(2500,0,0),FRotator::ZeroRotator);
  A->HalfHeight=B->HalfHeight=400; A->OnConstruction(A->GetActorTransform()); B->OnConstruction(B->GetActorTransform());
  P->SetActorLocation(Base+FVector(0,0,2000),false,nullptr,ETeleportType::TeleportPhysics);
  P->Gun->Remove(-1,true);
  Check(P->Gun->FireRay(false,Base+FVector(250,-300,0),FVector(-1,0,0)),TEXT("First portal on shared panel"));
  Check(P->Gun->FireRay(true,Base+FVector(250,300,0),FVector(-1,0,0)),TEXT("Opposite colour on same panel"));
  Check(!P->Gun->FireRay(true,Base+FVector(250,-280,0),FVector(-1,0,0)),TEXT("Overlapping circles rejected"));
  auto CheckHole=[&](float Y,bool bOpen) {
   FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(SharedPanelAudit),false,P);
   const bool bBlocked=GetWorld()->SweepSingleByChannel(Hit,Base+FVector(150,Y,0),Base+FVector(-50,Y,0),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(30,88),Q);
   Check(bBlocked!=bOpen,TEXT("Shared panel physical aperture matches placement"));
  };
  CheckHole(-300,true); CheckHole(300,true); CheckHole(0,false);
  Check(P->Gun->FireRay(false,Base+FVector(250,-450,0),FVector(-1,0,0)),TEXT("Move one portal on shared panel"));
  CheckHole(-450,true); CheckHole(-300,false); CheckHole(300,true);
  Check(P->Gun->Remove(0,true),TEXT("Remove one portal on shared panel"));
  CheckHole(-450,false); CheckHole(300,false);
  for(const FCase& Case:Cases) {
   UE_LOG(LogTemp,Display,TEXT("TRAVERSAL_CASE %s"),Case.Name);
   P->Gun->Remove(-1,true);
   P->SetActorLocation(Base+FVector(0,0,2000),false,nullptr,ETeleportType::TeleportPhysics);
   A->SetActorRotation(Case.From); B->SetActorRotation(Case.To);
   for(int YSign:{-1,1}) for(int ZSign:{-1,1}) {
    const FVector Edge=A->GetActorTransform().TransformPositionNoScale(FVector(0,YSign*(A->HalfWidth-1),ZSign*(A->HalfHeight-1)));
    Check(P->Gun->FireRay(false,Edge+A->GetActorForwardVector()*250,-A->GetActorForwardVector()),TEXT("Edge shot adjusts portal inward on oriented panel"));
    const auto* EdgeGate=P->Gun->GetPortal(0);
    const FVector Local=A->GetActorTransform().InverseTransformPositionNoScale(EdgeGate->GetActorLocation());
    Check(FMath::Abs(Local.Y)+EdgeGate->HalfWidth<=A->HalfWidth+.01f && FMath::Abs(Local.Z)+EdgeGate->HalfHeight<=A->HalfHeight+.01f &&
      !EdgeGate->GetActorLocation().Equals(Edge,1),TEXT("Adjusted circle stays entirely within panel"));
   }
   Check(P->Gun->FireRay(false,A->GetActorLocation()+A->GetActorForwardVector()*250,-A->GetActorForwardVector()),TEXT("Place entrance on oriented panel"));
   Check(P->Gun->FireRay(true,B->GetActorLocation()+B->GetActorForwardVector()*250,-B->GetActorForwardVector()),TEXT("Place exit on oriented panel"));
   auto* G=P->Gun->GetPortal(0); auto* H=P->Gun->GetPortal(1);
   if(!G->Linked) continue;
   const auto From=G->GetActorTransform(),To=H->GetActorTransform();
   const FVector N=G->GetActorForwardVector();
   const FVector Front=G->GetActorLocation()+N*200,Back=G->GetActorLocation()-N*5;
   FHitResult Sweep; FCollisionQueryParams Query(SCENE_QUERY_STAT(TraversalAperture),false,P);
   Check(!GetWorld()->SweepSingleByChannel(Sweep,Front,Back,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(30,88),Query),TEXT("Physical aperture admits upright capsule"));
   P->SetActorLocationAndRotation(Front,FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);
   P->GetController()->SetControlRotation(FRotator(-12,22,0)); P->ResetPortalTracking(true);
   P->SetActorLocation(Back,false,nullptr,ETeleportType::TeleportPhysics);
   const FVector ExpectedEye=PortalMath::Point(P->Camera->GetComponentLocation(),From,To);
   const FQuat ExpectedView=PortalMath::Rotation(From,To)*P->GetControlRotation().Quaternion();
   const FVector Incoming=-N*1700+G->GetActorRightVector()*130;
   P->GetCharacterMovement()->Velocity=Incoming;
   P->Tick(0);
   Check(P->GetActorLocation().Equals(PortalMath::Point(Back,From,To),.05),TEXT("Crossing preserves residual position"));
   Check(P->GetVelocity().Equals(PortalMath::Vector(Incoming,From,To),.05),TEXT("Crossing rotates velocity without changing speed"));
   Check(P->Camera->GetComponentLocation().Equals(ExpectedEye,.05),TEXT("Camera position is continuous through transformed portal"));
   Check(P->Camera->GetComponentQuat().Equals(ExpectedView,.0001),TEXT("Camera orientation is continuous at crossing"));
   Check(P->GetActorUpVector().Equals(FVector::UpVector,.001),TEXT("Collision capsule stays aligned with world gravity"));
   FVector LastEye=P->Camera->GetComponentLocation(); bool bSmooth=true;
   for(int32 Frame=0;Frame<120;++Frame) {
    P->Tick(1.f/60); const FVector Eye=P->Camera->GetComponentLocation();
    bSmooth&=FVector::Distance(Eye,LastEye)<25; LastEye=Eye;
   }
   Check(bSmooth&&P->Camera->GetComponentLocation().Equals(P->GetActorLocation()+FVector(0,0,64),.05)&&FMath::Abs(FRotator::NormalizeAxis(P->GetControlRotation().Roll))<.05,TEXT("Camera smoothly recovers upright eye pose"));
  }
  // Actual CharacterMovement gravity and collision, not a scripted position step.
  P->Gun->Remove(-1,true); A->SetActorRotation(FRotator(90,0,0)); B->SetActorRotation(FRotator::ZeroRotator);
  P->SetActorLocation(Base+FVector(0,0,2000),false,nullptr,ETeleportType::TeleportPhysics);
  P->Gun->FireRay(false,A->GetActorLocation()+FVector(0,0,250),FVector(0,0,-1));
  P->Gun->FireRay(true,B->GetActorLocation()+FVector(250,0,0),FVector(-1,0,0));
  P->SetActorLocationAndRotation(A->GetActorLocation()+FVector(0,0,350),FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);
  P->GetController()->SetControlRotation(FRotator(-50,0,0)); P->ResetPortalTracking(true);
  P->GetCharacterMovement()->Velocity=FVector(0,0,-100); P->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
  PhysicalExit=B->GetActorLocation(); Stage=1; Time=0;
 } else if(Stage==1) {
  if(P->GetActorLocation().X>PhysicalExit.X) {
   Check(P->GetVelocity().X>500,TEXT("Gravity-driven fall exits wall with horizontal fling momentum"));
   Check(P->GetCharacterMovement()->IsFalling(),TEXT("Gravity continues after floor-to-wall traversal"));
   Stage=2; Time=0;
  } else if(Time>3) { Check(false,TEXT("Gravity-driven fall reaches linked exit")); Stage=2; Time=0; }
 } else if(Stage==2&&Time>.4) {
  UE_LOG(LogTemp,Display,TEXT("PORTAL_TRAVERSAL_AUDIT %s checks=%d failures=%d"),Failures?TEXT("FAIL"):TEXT("PASS"),Checks,Failures);
  FPlatformMisc::RequestExitWithStatus(false,Failures?1:0); Stage=3;
 }
}
