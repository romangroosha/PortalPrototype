#include "PortalGunAudit.h"
#include "PortalGun.h"
#include "PortalLab.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LineBatchComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
APortalGunAudit::APortalGunAudit() { PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.TickGroup=TG_PostUpdateWork; }
void APortalGunAudit::BeginPlay() {
 Super::BeginPlay();
 Pawn=Cast<APortalPawn>(UGameplayStatics::GetPlayerPawn(this,0));
 check(Pawn && Pawn->Gun);
 Pawn->GetCharacterMovement()->DisableMovement(); LastPosition=Pawn->GetActorLocation();
}
bool APortalGunAudit::Check(bool Value,const TCHAR* Name) {
 ++Checks; if(!Value) ++Failures;
 UE_LOG(LogTemp,Display,TEXT("GUN_CHECK %s: %s"),Value?TEXT("PASS"):TEXT("FAIL"),Name);
 return Value;
}
bool APortalGunAudit::Blocked(APortalWall* Wall,const FVector& Center) const {
 FHitResult Hit; FCollisionQueryParams Query(SCENE_QUERY_STAT(GunCollisionAudit),false,Pawn);
 const FVector P=Center-FVector(0,0,49.85),N=Wall->GetActorForwardVector();
 return GetWorld()->SweepSingleByChannel(Hit,P+N*100,P-N*100,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(30,88),Query);
}
void APortalGunAudit::CheckActions() {
 auto* Gun=Pawn->Gun.Get(); auto* PC=Cast<APlayerController>(Pawn->GetController());
 APortalWall *East=nullptr,*West=nullptr,*North=nullptr;
 for(TActorIterator<APortalWall> It(GetWorld());It;++It) {
  if(It->GetActorLocation().X>900) East=*It;
  if(It->GetActorLocation().X< -900) West=*It;
  if(It->GetActorLocation().Y>800) North=*It;
 }
 check(East && West && North);
 const FVector Eye=Pawn->Camera->GetComponentLocation();
 Check(!Gun->IsPlaced(0)&&!Gun->IsPlaced(1),TEXT("Starts with no placed portals"));
 auto* Lines=GetWorld()->GetLineBatcher(UWorld::ELineBatcherType::WorldPersistent);
 const int BeforeReject=Lines->BatchedLines.Num();
 Check(!Gun->FireRay(false,Eye,FVector(0,0,-1)),TEXT("Floor rejects placement"));
 Check(Lines->BatchedLines.Num()==BeforeReject+1 && Lines->BatchedLines.Last().Start.Equals(Pawn->GetGunMuzzleLocation(),.01) && FMath::Abs(Lines->BatchedLines.Last().End.Z)<.1,
  TEXT("Rejected shot still draws beam from muzzle to impact"));
 const int BeforeMiss=Lines->BatchedLines.Num();
 const FVector MissOrigin(50000,50000,50000);
 Check(!Gun->FireRay(true,MissOrigin,FVector::UpVector),TEXT("Shot into empty space does not place portal"));
 Check(Lines->BatchedLines.Num()==BeforeMiss+1 && Lines->BatchedLines.Last().End.Equals(MissOrigin+FVector::UpVector*Gun->ShotRange,.01),TEXT("Miss still draws full range beam"));
 PC->SetControlRotation(FRotator::ZeroRotator);
 Check(Gun->Fire(false),TEXT("Primary fire places blue from player aim"));
 Check(Gun->IsPlaced(0)&&!Gun->GetPortal(0)->Linked,TEXT("Single portal is dormant"));
 Check(Blocked(East,Gun->GetPortal(0)->GetActorLocation()),TEXT("Dormant portal keeps physical wall closed"));
 PC->SetControlRotation(FRotator(0,180,0));
 Check(Gun->Fire(true),TEXT("Secondary fire places orange"));
 Check(Gun->GetPortal(0)->Linked==Gun->GetPortal(1)&&Gun->GetPortal(1)->Linked==Gun->GetPortal(0),TEXT("Pair links both directions"));
 Check(!Blocked(East,Gun->GetPortal(0)->GetActorLocation())&&!Blocked(West,Gun->GetPortal(1)->GetActorLocation()),TEXT("Both apertures pass capsule sweeps"));
 const FVector Old=Gun->GetPortal(0)->GetActorLocation();
 auto* Obstacle=GetWorld()->SpawnActor<AStaticMeshActor>(FVector(965,70,140),FRotator::ZeroRotator);
 Obstacle->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
 Obstacle->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
 Obstacle->SetActorScale3D(FVector(.2,.2,1));
 Check(!Gun->FireRay(false,Eye,(FVector(1000,0,140)-Eye).GetSafeNormal()),TEXT("Obstacle beside aim ray blocks full portal footprint"));
 Obstacle->SetActorEnableCollision(false); Obstacle->Destroy();
 Check(!Gun->FireRay(false,Eye,(FVector(1000,795,465)-Eye).GetSafeNormal()),TEXT("Solid corner obstruction rejects shot"));
 Check(Gun->GetPortal(0)->GetActorLocation().Equals(Old),TEXT("Invalid shot preserves previous portal"));
 Check(!Gun->FireRay(false,Eye,(FVector(-1000,200,140)-Eye).GetSafeNormal()),TEXT("Overlapping portal rims cannot overwrite the other colour"));
 Pawn->SetActorLocation(FVector(990,0,90.15),false,nullptr,ETeleportType::TeleportPhysics); Pawn->ResetPortalTracking();
 Check(!Gun->Remove(),TEXT("Clear is refused while capsule occupies aperture"));
 Check(East->HasOpening()&&West->HasOpening(),TEXT("Refused clear leaves passage unchanged"));
 Pawn->SetActorLocation(FVector(0,0,90.15),false,nullptr,ETeleportType::TeleportPhysics); Pawn->ResetPortalTracking();
 Check(Gun->FireRay(false,Eye,(FVector(0,900,140)-Eye).GetSafeNormal()),TEXT("Blue can move to another panel"));
 Check(Blocked(East,Old)&&!Blocked(North,Gun->GetPortal(0)->GetActorLocation()),TEXT("Moving restores old wall and opens new wall"));
 Check(Gun->Remove(1),TEXT("Remove orange independently"));
 Check(Gun->IsPlaced(0)&&!Gun->IsPlaced(1)&&!Gun->GetPortal(0)->Linked,TEXT("Remaining blue becomes dormant"));
 Check(Blocked(North,Gun->GetPortal(0)->GetActorLocation()),TEXT("Unlinked aperture closes"));
 Check(Gun->Remove(),TEXT("Clear both"));
 Check(!Gun->IsPlaced(0)&&!Gun->IsPlaced(1)&&Gun->GetPortal(0)->IsHidden()&&Gun->GetPortal(1)->IsHidden(),TEXT("Cleared portals are hidden"));
 PC->SetControlRotation(FRotator::ZeroRotator);
 Check(Gun->Fire(false),TEXT("Can place blue again after clearing"));
 LastPosition=Pawn->GetActorLocation();
}
void APortalGunAudit::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds); Seconds+=DeltaSeconds;
 if(Stage==0 && Seconds>1) {
  if(!Check(Pawn->Gun->GetPortal(0)!=nullptr,TEXT("Gun initializes after world BeginPlay"))) {
   Stage=99; FPlatformMisc::RequestExitWithStatus(false,1); return;
  }
  CheckActions(); Stage=1;
 }
 if(Stage==1 && Seconds>1.5) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("GunDormant.png"),false,false); Stage=2; }
 if(Stage==2 && Seconds>2) {
  auto* PC=Cast<APlayerController>(Pawn->GetController()); PC->SetControlRotation(FRotator(0,180,0));
  Check(Pawn->Gun->Fire(true),TEXT("Can relink after clearing")); PC->SetControlRotation(FRotator::ZeroRotator);
  Pawn->GetCharacterMovement()->SetMovementMode(MOVE_Walking); Stage=3;
 }
 if(Stage==3 && Seconds>2.5) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("GunLinked.png"),false,false); Stage=4; }
 if(Seconds>3.5 && Seconds<11.5) Pawn->AddMovementInput(FVector::ForwardVector,1);
 if(Pawn->GetActorLocation().X-LastPosition.X< -1500) ++Crossings;
 LastPosition=Pawn->GetActorLocation();
 FVector View; FRotator Rotation; Cast<APlayerController>(Pawn->GetController())->GetPlayerViewPoint(View,Rotation);
 if(Seconds>3.5) MaxCameraError=FMath::Max(MaxCameraError,FVector::Distance(View,Pawn->Camera->GetComponentLocation()));
 if(Stage==4 && Seconds>12) {
  Check(Crossings>=2,TEXT("Player walks through gun-placed portals repeatedly"));
  Check(MaxCameraError<.01,TEXT("Camera follows traversal without stale position"));
  Check(Pawn->Gun->Remove(),TEXT("Clear after traversing"));
  for(TActorIterator<APortalWall> It(GetWorld());It;++It) Check(!It->HasOpening(),TEXT("All panels closed after clear"));
  UE_LOG(LogTemp,Display,TEXT("PORTAL_GUN_AUDIT %s checks=%d failures=%d crossings=%d camera_error=%.6f"),Failures?TEXT("FAIL"):TEXT("PASS"),Checks,Failures,Crossings,MaxCameraError);
  Stage=5; FPlatformMisc::RequestExitWithStatus(false,Failures?1:0);
 }
}
