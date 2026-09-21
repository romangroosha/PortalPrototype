#include "PortalFoundationAudit.h"
#include "PortalMechanisms.h"
#include "PortalPuzzle.h"
#include "PortalLab.h"
#include "PortalGun.h"
#include "PortalTrace.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
APortalFoundationAudit::APortalFoundationAudit() { PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.TickGroup=TG_PostUpdateWork; }
void APortalFoundationAudit::Check(bool Value,const TCHAR* Name) { ++Checks; if(!Value) ++Failures; UE_LOG(LogTemp,Display,TEXT("FOUNDATION_CHECK %s: %s"),Value?TEXT("PASS"):TEXT("FAIL"),Name); }
void APortalFoundationAudit::Tick(float Dt) {
 Super::Tick(Dt); Time+=FMath::Min(Dt,.05f);
 auto Find=[&](FName Tag)->AActor* { for(TActorIterator<AActor> It(GetWorld());It;++It) if(It->ActorHasTag(Tag)) return *It; return nullptr; };
 auto* A=Cast<APortalPressureButton>(Find(TEXT("InputA"))); auto* B=Cast<APortalPressureButton>(Find(TEXT("InputB")));
 auto* LoadA=Cast<APortalCarryable>(Find(TEXT("LoadA"))); auto* LoadB=Cast<APortalCarryable>(Find(TEXT("LoadB")));
 auto* OnlyA=Cast<APortalDoor>(Find(TEXT("OnlyA"))); auto* OnlyB=Cast<APortalDoor>(Find(TEXT("OnlyB"))); auto* Both=Cast<APortalDoor>(Find(TEXT("Both")));
 auto* Exit=Cast<APortalLevelExit>(Find(TEXT("Exit")));
 auto* SurfaceA=Cast<APortalWall>(Find(TEXT("SurfaceA"))); auto* SurfaceB=Cast<APortalWall>(Find(TEXT("SurfaceB"))); auto* Blocked=Cast<APortalWall>(Find(TEXT("NonPortalSurface")));
 auto* P=Cast<APortalPawn>(UGameplayStatics::GetPlayerPawn(this,0));
 if(!A||!B||!LoadA||!LoadB||!OnlyA||!OnlyB||!Both||!Exit||!P||!SurfaceA||!SurfaceB||!Blocked) { Check(false,TEXT("Fixture contains reusable objects")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
 auto Drop=[&](APortalCarryable* Load,APortalPressureButton* Button) {
  Load->SetHeld(nullptr); Load->SetActorLocation(Button->GetActorLocation()+Button->GetActorUpVector()*(Load->ExtentAlong(Button->GetActorUpVector())+Button->Thickness*.5+15),false,nullptr,ETeleportType::TeleportPhysics);
  Load->PreviousPosition=Load->GetActorLocation();
 };
 if(Stage==0 && Time>3) {
  Check(!A->bActive&&!B->bActive&&!Both->Input->Evaluate(),TEXT("Independent assembly starts unpowered"));
  Check(!LoadA->IsA<APortalCube>() && LoadA->MassKg!=LoadB->MassKg,TEXT("Non-cube carryable and distinct masses"));
  Check(!A->GetActorLocation().Equals(FVector(400,-360,12)) && !A->GetActorRotation().IsNearlyZero(),TEXT("Fixture translated and rotated from original map"));
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("FoundationAudit.png"),false,false);
  FVector Center;
  Check(!Blocked->FindPlacement(Blocked->GetActorLocation(),Center),TEXT("Portal permission is a surface parameter"));
  Check(!SurfaceA->FindPlacement(SurfaceA->GetActorLocation(),Center,FVector2D(900,900)),TEXT("Oversized aperture is rejected"));
  Check(P->Gun->FireRay(false,SurfaceA->GetActorLocation()+SurfaceA->GetActorForwardVector()*250,-SurfaceA->GetActorForwardVector()),TEXT("Place portal on rotated surface A"));
  Check(P->Gun->FireRay(true,SurfaceB->GetActorLocation()+SurfaceB->GetActorForwardVector()*250,-SurfaceB->GetActorForwardVector()),TEXT("Place portal on independent surface B"));
  Check(P->Gun->GetPortal(0)->Linked==P->Gun->GetPortal(1),TEXT("Reusable pair links outside original map"));
  auto* G0=P->Gun->GetPortal(0); auto* G1=P->Gun->GetPortal(1);
  auto* Remote=GetWorld()->SpawnActor<APortalCarryable>(G1->GetActorLocation()+G1->GetActorForwardVector()*100,FRotator::ZeroRotator);
  Remote->Mesh->SetSimulatePhysics(false);
  FHitResult PlacementHit; FCollisionQueryParams PlacementQuery(SCENE_QUERY_STAT(PortalSurfaceAudit),false,P);
  Check(PortalTrace::Line(GetWorld(),PlacementHit,G0->GetActorLocation()+G0->GetActorForwardVector()*100,-G0->GetActorForwardVector(),240,ECC_Visibility,PlacementQuery)&&PlacementHit.GetActor()==Remote,TEXT("Interaction crosses authored wall aim surface through actual aperture"));
  Remote->Destroy();
  Drop(LoadA,B); ++Stage; Time=0;
 } else if(Stage==1 && Time>2) {
  Check(!B->bActive && OnlyB->OpenAmount==0,TEXT("Mass threshold rejects six kg on ten kg button"));
  Drop(LoadA,A); ++Stage; Time=0;
 } else if(Stage==2 && Time>2) {
  Check(A->bActive && !B->bActive && OnlyA->OpenAmount>.99 && OnlyB->OpenAmount==0 && Both->OpenAmount==0,TEXT("First button only drives its own door; AND remains closed"));
  Both->Input->Rule=EPortalSignalRule::Any; Both->Input->RefreshBindings(); ++Stage; Time=0;
 } else if(Stage==3 && Time>2) {
  Check(Both->OpenAmount>.99,TEXT("OR combines the same sources without custom map code"));
  Both->Input->Rule=EPortalSignalRule::All; Both->Input->bInvert=true; Both->Input->RefreshBindings();
  Check(Both->Input->bValue,TEXT("Inverted AND signal exposed through component"));
  Both->Input->bInvert=false; Both->Input->RefreshBindings(); Drop(LoadB,B); ++Stage; Time=0;
 } else if(Stage==4 && Time>2) {
  Check(A->bActive&&B->bActive&&OnlyA->OpenAmount>.99&&OnlyB->OpenAmount>.99&&Both->OpenAmount>.99,TEXT("Two objects power both independent doors and combined exit"));
  Check(OnlyA->Panel->GetRelativeLocation().Equals(OnlyA->OpenOffset,.1),TEXT("Sideways movement follows door local open offset"));
  LoadB->SetHeld(P); P->HeldObject=LoadB; ++Stage; Time=0;
 } else if(Stage==5 && Time>2) {
  Check(!B->bActive&&A->bActive&&OnlyB->OpenAmount==0&&OnlyA->OpenAmount>.99,TEXT("Picking up one object affects only connected signals"));
  P->HeldObject=nullptr; Drop(LoadB,B); ++Stage; Time=0;
 } else if(Stage==6 && Time>2) {
  P->SetActorLocation(Exit->GetActorLocation(),false,nullptr,ETeleportType::TeleportPhysics); P->GetCharacterMovement()->StopMovementImmediately(); P->ResetPortalTracking(); ++Stage; Time=0;
 } else if(Stage==7 && Time>1) {
  Check(Exit->bComplete,TEXT("Translated exit uses overlap and configured signals"));
  GetWorld()->GetSubsystem<UPortalLevelSubsystem>()->ResetLevelObjects();
  Check(!Exit->bComplete && !A->bActive && !B->bActive && Both->OpenAmount==0,TEXT("Generic lifecycle resets every mechanism"));
  Check(LoadA->GetActorTransform().Equals(LoadA->SpawnTransform,.1)&&LoadB->GetActorTransform().Equals(LoadB->SpawnTransform,.1),TEXT("Reset restores full transforms for both shapes"));
  auto* Timer=GetWorld()->SpawnActor<APortalTimedButton>(A->GetActorLocation()+FVector(0,0,300),FRotator::ZeroRotator);
  Timer->Tags.Add(TEXT("AuditTimer")); Timer->Duration=.5;
  OnlyA->Input->Sources={Timer}; OnlyA->Input->RefreshBindings();
  Check(IPortalInteractable::Execute_TryInteract(Timer,P)&&Timer->bActive&&OnlyA->Input->bValue,TEXT("Generic interaction activates timed switch and event input"));
  ++Stage; Time=0;
 } else if(Stage==8 && Time>1) {
  auto* Timer=Cast<APortalTimedButton>(Find(TEXT("AuditTimer")));
  Check(Timer&&!Timer->bActive&&!OnlyA->Input->bValue,TEXT("Timer expiration deactivates connected input"));
  if(Timer) {
   Timer->bToggle=true; IPortalInteractable::Execute_TryInteract(Timer,P);
   Check(Timer->bActive,TEXT("Switch supports persistent toggle mode"));
   IPortalInteractable::Execute_TryInteract(Timer,P); Check(!Timer->bActive,TEXT("Second interaction toggles off"));
   Timer->Destroy();
  }
  // An isolated assembly checks the same trace used by the player's F key.
  const FVector Origin(30000,0,10000);
  auto* Entry=GetWorld()->SpawnActor<APortalGate>(Origin,FRotator::ZeroRotator);
  auto* Destination=GetWorld()->SpawnActor<APortalGate>(Origin+FVector(2000,0,0),FRotator(0,90,0));
  Entry->Linked=Destination; Destination->Linked=Entry;
  auto* Item=GetWorld()->SpawnActor<APortalCarryable>(Destination->GetActorLocation()+FVector(0,100,0),FRotator::ZeroRotator);
  Item->Mesh->SetSimulatePhysics(false);
  FCollisionQueryParams Query(SCENE_QUERY_STAT(PortalTraceAudit),false,P);
  FHitResult Ray;
  const FVector Eye=Origin+FVector(100,0,0);
  auto Trace=[&](float Range=240,int32 Hops=8) { return PortalTrace::Line(GetWorld(),Ray,Eye,-FVector::ForwardVector,Range,ECC_Visibility,Query,Hops); };
  Check(Trace()&&Ray.GetActor()==Item&&FMath::IsNearlyEqual(Ray.Distance,168.f,1.f),TEXT("Interaction ray crosses rotated portal and measures total path"));
  Check(!Trace(150),TEXT("Interaction range is shared by all portal segments"));
  Check(!Trace(240,0),TEXT("Exhausted portal hop budget stops interaction"));
  Destination->Linked=nullptr; Entry->Linked=nullptr;
  Check(!Trace(),TEXT("Unlinked gate never redirects interaction"));
  Entry->Linked=Destination; Destination->Linked=Entry;
  Check(!PortalTrace::Line(GetWorld(),Ray,Origin-FVector(100,0,0),FVector::ForwardVector,240,ECC_Visibility,Query),TEXT("Back of portal does not redirect ray"));
  auto* Obstacle=GetWorld()->SpawnActor<APortalCarryable>(Origin+FVector(50,0,0),FRotator::ZeroRotator);
  Obstacle->Mesh->SetSimulatePhysics(false);
  Check(Trace()&&Ray.GetActor()==Obstacle,TEXT("Obstacle before portal blocks interaction"));
  Obstacle->SetActorLocation(Destination->GetActorLocation()+FVector(0,50,0));
  Check(Trace()&&Ray.GetActor()==Obstacle,TEXT("Obstacle after portal blocks remote object"));
  Obstacle->Destroy();
  const FVector OldPosition=P->GetActorLocation(); const FRotator OldView=P->GetControlRotation();
  P->SetActorLocation(Eye-FVector(0,0,64),false,nullptr,ETeleportType::TeleportPhysics);
  P->GetController()->SetControlRotation(FRotator(0,180,0));
  Item->bCanTraversePortals=false;
  P->Interact();
  Check(!P->HeldObject&&!Item->Holder,TEXT("Non-traversable item cannot be pulled through portal"));
  Item->bCanTraversePortals=true;
  P->Interact();
  Check(P->HeldObject==Item&&Item->Holder==P,TEXT("Player interaction picks up remote item through portal"));
  P->Interact();
  Check(!P->HeldObject&&!Item->Holder,TEXT("Remote pickup releases through normal interaction lifecycle"));
  Item->Destroy();
  auto* RemoteSwitch=GetWorld()->SpawnActor<APortalTimedButton>(Destination->GetActorLocation()+FVector(0,100,-50),FRotator::ZeroRotator);
  P->Interact();
  Check(RemoteSwitch->bActive,TEXT("Player interaction activates remote timed button through portal"));
  RemoteSwitch->Destroy();
  auto* SecondEntry=GetWorld()->SpawnActor<APortalGate>(Destination->GetActorLocation()+FVector(0,80,0),FRotator(0,-90,0));
  auto* LastExit=GetWorld()->SpawnActor<APortalGate>(Origin+FVector(4000,0,0),FRotator::ZeroRotator);
  SecondEntry->Linked=LastExit; LastExit->Linked=SecondEntry;
  auto* ChainItem=GetWorld()->SpawnActor<APortalCarryable>(LastExit->GetActorLocation()+FVector(50,0,0),FRotator::ZeroRotator);
  ChainItem->Mesh->SetSimulatePhysics(false); ChainItem->CarryDistance=250;
  LastExit->HalfWidth=20;
  P->Interact();
  Check(!P->HeldObject,TEXT("Visible item cannot be acquired through undersized exit"));
  LastExit->HalfWidth=90;
  P->Interact();
  Check(P->HeldObject==ChainItem,TEXT("Player acquires item through two consecutive pairs"));
  P->ResetPortalTracking(); P->Tick(0);
  Check(ChainItem->GetActorLocation().Equals(LastExit->GetActorLocation()+FVector(70,0,-20),.2),TEXT("Held item follows the complete two-pair path"));
  Check(ChainItem->GetActorQuat().Equals(FQuat::Identity,.001),TEXT("Held orientation composes both portal rotations"));
  FCollisionQueryParams CarryQuery=Query; CarryQuery.AddIgnoredActor(ChainItem);
  FVector BoxTarget; FQuat BoxRotation; int32 BoxHops=0;
  auto Carry=[&](bool bAllowed=true,int32 Limit=8) {
   BoxTarget=Eye-FVector(250,0,0); BoxRotation=FQuat::Identity;
   PortalTrace::MoveBox(GetWorld(),Eye,BoxTarget,BoxRotation,FVector(10),CarryQuery,bAllowed,Limit,&BoxHops);
  };
  Carry();
  Check(BoxHops==2&&BoxTarget.Equals(LastExit->GetActorLocation()+FVector(70,0,0),.1),TEXT("Box solver and interaction share nearest-aperture path"));
  Carry(false);
  Check(BoxHops==0&&BoxTarget.X>Origin.X+10,TEXT("Non-traversable held object stops before entrance"));
  Carry(true,1);
  Check(BoxHops==1&&BoxTarget.Y<SecondEntry->GetActorLocation().Y-10,TEXT("Carry hop limit stops safely before next aperture"));
  LastExit->HalfWidth=5; Carry();
  Check(BoxHops==1&&BoxTarget.Y<SecondEntry->GetActorLocation().Y-10,TEXT("Box cannot enter a pair whose exit is too small"));
  LastExit->HalfWidth=90;
  auto* MiddleObstacle=GetWorld()->SpawnActor<APortalCarryable>(Destination->GetActorLocation()+FVector(0,40,0),FRotator::ZeroRotator);
  MiddleObstacle->Mesh->SetSimulatePhysics(false); MiddleObstacle->SetActorScale3D(FVector(.1));
  Carry();
  Check(BoxHops==1&&BoxTarget.Y<MiddleObstacle->GetActorLocation().Y-10,TEXT("Sweep detects obstacle on intermediate portal segment"));
  MiddleObstacle->Destroy();
  LastExit->SetActorRotation(FRotator(90,0,0)); Carry();
  Check(BoxHops==2&&BoxTarget.Equals(LastExit->GetActorLocation()+FVector(0,0,70),.1),TEXT("Held path supports floor-facing exit and transformed box"));
  P->Interact(); ChainItem->Destroy(); SecondEntry->Destroy(); LastExit->Destroy();
  P->SetActorLocation(OldPosition,false,nullptr,ETeleportType::TeleportPhysics); P->GetController()->SetControlRotation(OldView); P->ResetPortalTracking();
  Destination->SetActorLocationAndRotation(Origin+FVector(200,0,0),FRotator(0,180,0));
  Check(!Trace(10000,4),TEXT("Cyclic portal ray terminates at hop budget"));
  BoxTarget=Eye-FVector(10000,0,0); BoxRotation=FQuat::Identity;
  PortalTrace::MoveBox(GetWorld(),Eye,BoxTarget,BoxRotation,FVector(10),Query,true,4,&BoxHops);
  Check(BoxHops==4&&BoxTarget.X>Origin.X+10&&BoxTarget.X<Origin.X+200,TEXT("Cyclic carry path terminates at reachable pose"));
  Entry->Destroy(); Destination->Destroy();
  UE_LOG(LogTemp,Display,TEXT("PORTAL_FOUNDATION_AUDIT %s checks=%d failures=%d"),Failures?TEXT("FAIL"):TEXT("PASS"),Checks,Failures);
  FPlatformMisc::RequestExitWithStatus(false,Failures?1:0); ++Stage;
 }
}
