#include "PortalPuzzleAudit.h"
#include "PortalPuzzle.h"
#include "PortalGun.h"
#include "PortalLab.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
APortalPuzzleAudit::APortalPuzzleAudit() { PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.TickGroup=TG_PostUpdateWork; }
void APortalPuzzleAudit::Check(bool Value,const TCHAR* Name) { if(!Value) ++Failures; UE_LOG(LogTemp,Display,TEXT("PUZZLE_CHECK %s: %s"),Value?TEXT("PASS"):TEXT("FAIL"),Name); }
void APortalPuzzleAudit::Shot(const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("PuzzleAudit")/Name,false,false); }
void APortalPuzzleAudit::Tick(float Dt) {
 Super::Tick(Dt); Time+=FMath::Min(Dt,.05f);
 auto* P=Cast<APortalPawn>(UGameplayStatics::GetPlayerPawn(this,0)); if(!P) return;
 auto* PC=Cast<APlayerController>(P->GetController());
 APortalLevelExit* Puzzle=nullptr; APortalPressureButton* Button=nullptr; APortalDoor* Door=nullptr; APortalCube* Cube=nullptr;
 for(TActorIterator<APortalLevelExit> It(GetWorld());It;++It) Puzzle=*It;
 for(TActorIterator<APortalPressureButton> It(GetWorld());It;++It) Button=*It;
 for(TActorIterator<APortalDoor> It(GetWorld());It;++It) Door=*It;
 for(TActorIterator<APortalCube> It(GetWorld());It;++It) Cube=*It;
 if(!Puzzle||!Cube||!Button||!Door) { Check(false,TEXT("Puzzle actors exist")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
 auto Place=[&](FVector V,FRotator R) { P->SetActorLocation(V,false,nullptr,ETeleportType::TeleportPhysics); PC->SetControlRotation(R); P->GetCharacterMovement()->StopMovementImmediately(); P->ResetPortalTracking(); };
 if(Stage==0 && Time>3) {
  Check(!Button->bActive && !Puzzle->bComplete && Door->OpenAmount==0,TEXT("Chamber begins locked"));
  Place(FVector(940,0,88),FRotator::ZeroRotator); Door->Tick(.1f);
  Check(Door->OpenAmount==0,TEXT("Approaching locked door cannot open it without cube"));
  Place(FVector(0,0,88),FRotator(0,90,0));
  Check(P->GetMesh()->GetSkeletalMeshAsset()!=nullptr,TEXT("UE Manny mesh loaded"));
  FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(PuzzleAudit),false,P);
  Check(GetWorld()->SweepSingleByChannel(Hit,FVector(0,700,88),FVector(0,1400,88),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(30,88),Q),TEXT("Observation opening blocks walking/jumping route"));
  Check(GetWorld()->SweepSingleByChannel(Hit,FVector(0,700,240),FVector(0,1400,240),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(30,88),Q),TEXT("Vault barrier also blocks a jump through the window"));
  Shot(TEXT("01_chamber.png")); ++Stage; Time=0;
 } else if(Stage==1 && Time>1) {
  Check(P->Gun->FireRay(false,FVector(0,0,152),FVector(-990,0,140)-FVector(0,0,152)),TEXT("Place main chamber portal"));
  Check(P->Gun->FireRay(true,FVector(0,0,152),FVector(0,2010,140)-FVector(0,0,152)),TEXT("Shoot vault portal through observation opening"));
  Place(FVector(0,0,88),FRotator(0,180,0)); Stage=101; Time=0;
 } else if(Stage==101 && Time>.5) {
  if(!bShot) {
   Shot(*FString::Printf(TEXT("distance_%d.png"),DistanceStep)); bShot=true;
   auto* G=P->Gun->GetPortal(DistanceStep==3?1:0);
   UE_LOG(LogTemp,Display,TEXT("PORTAL_DISTANCE step=%d position=%s lastCapture=%llu frame=%llu passes=%d"),DistanceStep,*P->GetActorLocation().ToString(),G->LastCaptureFrame,GFrameCounter,G->LastRenderPasses);
  }
  if(Time>1) {
   const FVector Positions[]={FVector(-400,0,88),FVector(-750,0,88),FVector(0,1450,88),FVector(-400,150,88),FVector(-850,0,88)};
   const float Yaws[]={180,180,90,-165.735,180};
   Place(Positions[DistanceStep],FRotator(0,Yaws[DistanceStep],0));
   ++DistanceStep; if(DistanceStep==5) Stage=2;
   Time=0; bShot=false;
  }
 } else if(Stage==2 && Time>.5) {
  if(!bShot) { Shot(TEXT("02_portal.png")); bShot=true; }
  if(Time>1) { Place(FVector(-935,0,88),FRotator(0,180,0)); P->GetCharacterMovement()->DisableMovement(); ++Stage; Time=0; bShot=false; }
 } else if(Stage==3 && Time>.5) {
  if(!bShot) { Shot(TEXT("03_gun_straddling.png")); bShot=true; }
  if(Time>1) { Place(FVector(-982,0,88),FRotator(0,180,0)); ++Stage; Time=0; bShot=false; }
 } else if(Stage==4 && Time>.5) {
  if(!bShot) { Shot(TEXT("04_gun_near.png")); bShot=true; }
  if(Time>1) { P->GetCharacterMovement()->SetMovementMode(MOVE_Walking); ++Stage; Time=0; bShot=false; }
 } else if(Stage==5) {
  P->AddMovementInput(FVector(-1,0,0));
  if(P->GetActorLocation().Y>1300) { Check(true,TEXT("Walk through portal into cube vault")); P->GetCharacterMovement()->StopMovementImmediately(); ++Stage; Time=0; }
  else if(Time>4) { Check(false,TEXT("Walk into vault timed out")); ++Stage; Time=0; }
 } else if(Stage==6 && Time>1) {
  const FVector Eye=FVector(0,1800,152); Place(FVector(0,1800,88),(Cube->GetActorLocation()-Eye).Rotation());
  ++Stage; Time=0;
 } else if(Stage==7 && Time>.3) { P->Interact(); Check(P->HeldObject==Cube,TEXT("Pick up cube with interaction trace")); Place(FVector(0,1860,88),FRotator(0,90,0)); ++Stage; Time=0;
 } else if(Stage==8) {
  P->AddMovementInput(FVector(0,1,0));
  if(P->GetActorLocation().X< -700) { Check(P->HeldObject==Cube,TEXT("Return through portal carrying cube")); P->GetCharacterMovement()->StopMovementImmediately(); ++Stage; Time=0; }
  else if(Time>4) { Check(false,TEXT("Return with cube timed out")); ++Stage; Time=0; }
 } else if(Stage==9 && Time>.5) {
  Place(FVector(245,-360,88),FRotator(-45,0,0)); ++Stage; Time=0;
 } else if(Stage==10 && Time>.5) {
  Check(!Button->bActive,TEXT("Held cube does not activate button"));
  // Drop using the same input path; position the player so the held cube lands centrally.
  Place(FVector(312,-360,88),FRotator(-45,0,0)); ++Stage; Time=0;
 } else if(Stage==11 && Time>.5) { P->Interact(); ++Stage; Time=0;
 } else if(Stage==12 && Time>3) {
  Check(!Cube->Holder && Button->bActive && Door->OpenAmount>.99,TEXT("Dropped cube settles on button and opens door"));
  UE_LOG(LogTemp,Display,TEXT("Dropped cube location=%s velocity=%s"),*Cube->GetActorLocation().ToString(),*Cube->GetVelocity().ToString());
  Place(FVector(650,-280,88),FRotator(0,35,0)); ++Stage; Time=0;
 } else if(Stage==13 && Time>.5) {
  if(!bShot) { Shot(TEXT("05_door_open.png")); bShot=true; }
  if(Time>1) { Cube->SetHeld(P); P->HeldObject=Cube; Place(FVector(0,0,88),FRotator::ZeroRotator); ++Stage; Time=0; bShot=false; }
 } else if(Stage==14 && Time>2) {
  Check(!Button->bActive && Door->OpenAmount<.01,TEXT("Removing cube closes door"));
  P->HeldObject=nullptr; Cube->SetHeld(nullptr); Cube->SetActorLocation(Button->GetActorLocation()+FVector(0,0,42),false,nullptr,ETeleportType::TeleportPhysics); Cube->Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
  Place(FVector(800,0,88),FRotator::ZeroRotator); ++Stage; Time=0;
 } else if(Stage==15 && Time>2) { P->AddMovementInput(FVector(1,0,0));
  if(Puzzle->bComplete || Time>6) { Check(Puzzle->bComplete,TEXT("Walk through open door to finish trigger")); Shot(TEXT("06_complete.png")); ++Stage; Time=0; }
 } else if(Stage==16 && Time>1) {
  Place(FVector(0,0,88),FRotator::ZeroRotator);
  Cube->SetHeld(nullptr);
  Cube->SetActorLocationAndRotation(Button->GetActorLocation()+FVector(0,0,110),FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);
  Stage=102; Time=0; PeakUpSpeed=0; bLostButton=false;
 } else if(Stage==102) {
  PeakUpSpeed=FMath::Max(PeakUpSpeed,float(Cube->Mesh->GetPhysicsLinearVelocity().Z));
  if(Time>2 && !Button->bActive) bLostButton=true;
  if(Time>10) {
   Check(!bLostButton && Button->bActive && Cube->GetVelocity().Size()<5,TEXT("Cube stays on plate for ten seconds after center/edge contact"));
   Check(PeakUpSpeed<60,TEXT("Button does not inject a launching impulse"));
   UE_LOG(LogTemp,Display,TEXT("BUTTON_STRESS case=%d peakUp=%.3f position=%s"),DropCase,PeakUpSpeed,*Cube->GetActorLocation().ToString());
   ++DropCase;
   if(DropCase<3) {
    Cube->SetHeld(nullptr);
    Cube->SetActorLocationAndRotation(Button->GetActorLocation()+FVector(DropCase==1?35:-35,0,85),FRotator(0,35,0),false,nullptr,ETeleportType::TeleportPhysics);
    Cube->Mesh->SetPhysicsLinearVelocity(FVector(DropCase==1?-15:15,0,-30));
    Time=0; PeakUpSpeed=0; bLostButton=false;
   } else { Stage=103; Time=0; }
  }
 } else if(Stage==103 && Time>1) {
  GetWorld()->GetSubsystem<UPortalLevelSubsystem>()->ResetLevelObjects(); Check(!Puzzle->bComplete && !Button->bActive && FVector::Dist(Cube->GetActorLocation(),Cube->SpawnPosition)<1,TEXT("Restart restores cube and puzzle"));
  UE_LOG(LogTemp,Display,TEXT("PORTAL_PUZZLE_AUDIT %s: %d failures"),Failures?TEXT("FAIL"):TEXT("PASS"),Failures);
  FPlatformMisc::RequestExitWithStatus(false,Failures?1:0); ++Stage;
 }
}
