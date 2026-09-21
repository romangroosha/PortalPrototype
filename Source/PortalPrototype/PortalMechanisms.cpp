#include "PortalMechanisms.h"
#include "PortalPuzzle.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

APortalSignal::APortalSignal() { RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root")); }
void APortalSignal::BeginPlay() { Super::BeginPlay(); SetActive(bInitiallyActive); }
void APortalSignal::EndPlay(const EEndPlayReason::Type Reason) { SetActive(false); Super::EndPlay(Reason); }
void APortalSignal::SetActive(bool bValue) { if(bActive!=bValue) { bActive=bValue; OnSignalChanged.Broadcast(bActive); } }
void APortalSignal::ResetGameplay_Implementation() { SetActive(bInitiallyActive); }
APortalTimedButton::APortalTimedButton() {
 PrimaryActorTick.bCanEverTick=true;
 SwitchMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Switch")); SwitchMesh->SetupAttachment(RootComponent);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> Orange(TEXT("/Game/Materials/M_Orange.M_Orange"));
 SwitchMesh->SetStaticMesh(Shape.Object); SwitchMesh->SetMaterial(0,Orange.Object);
 SwitchMesh->SetRelativeScale3D(FVector(.5,.5,1)); SwitchMesh->SetRelativeLocation(FVector(0,0,50)); SwitchMesh->SetCollisionProfileName(TEXT("BlockAll"));
}
void APortalTimedButton::BeginPlay() { Super::BeginPlay(); Remaining=bInitiallyActive?Duration:0; }
bool APortalTimedButton::TryInteract_Implementation(APawn* User) {
 if(!IsValid(User)) return false;
 SetActive(bToggle?!bActive:true); Remaining=bActive?Duration:0; return true;
}
void APortalTimedButton::Tick(float Dt) {
 Super::Tick(Dt);
 if(bActive&&!bToggle) { Remaining=FMath::Max(0.f,Remaining-Dt); if(Remaining<=0) SetActive(false); }
}
void APortalTimedButton::ResetGameplay_Implementation() { Super::ResetGameplay_Implementation(); Remaining=bInitiallyActive?Duration:0; }
UPortalSignalInput::UPortalSignalInput() { PrimaryComponentTick.bCanEverTick=false; }
void UPortalSignalInput::BeginPlay() { Super::BeginPlay(); RefreshBindings(); }
void UPortalSignalInput::EndPlay(const EEndPlayReason::Type Reason) {
 for(auto Source:BoundSources) if(IsValid(Source)) Source->OnSignalChanged.RemoveDynamic(this,&UPortalSignalInput::SourceChanged);
 BoundSources.Reset(); Super::EndPlay(Reason);
}
bool UPortalSignalInput::Evaluate() const {
 bool Value=Sources.Num()==0?bValueWithoutSources:Rule==EPortalSignalRule::All;
 for(auto Source:Sources) {
  const bool Active=IsValid(Source)&&Source->bActive;
  Value=Rule==EPortalSignalRule::All?(Value&&Active):(Value||Active);
 }
 return bInvert?!Value:Value;
}
void UPortalSignalInput::SourceChanged(bool) {
 const bool NewValue=Evaluate(); if(NewValue!=bValue) { bValue=NewValue; OnValueChanged.Broadcast(bValue); }
}
void UPortalSignalInput::RefreshBindings() {
 for(auto Source:BoundSources) if(IsValid(Source)) Source->OnSignalChanged.RemoveDynamic(this,&UPortalSignalInput::SourceChanged);
 BoundSources.Reset();
 for(auto Source:Sources) if(IsValid(Source)) { Source->OnSignalChanged.AddUniqueDynamic(this,&UPortalSignalInput::SourceChanged); BoundSources.AddUnique(Source); }
 SourceChanged(false);
}

APortalPressureButton::APortalPressureButton() {
 PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.TickGroup=TG_PostPhysics;
 Plate=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plate")); Plate->SetupAttachment(RootComponent);
 Sensor=CreateDefaultSubobject<UBoxComponent>(TEXT("Sensor")); Sensor->SetupAttachment(RootComponent);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> Orange(TEXT("/Game/Materials/M_Orange.M_Orange"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> Blue(TEXT("/Game/Materials/M_Blue.M_Blue"));
 Plate->SetStaticMesh(Shape.Object); Plate->SetCollisionProfileName(TEXT("BlockAll"));
 ReleasedMaterial=Orange.Object; PressedMaterial=Blue.Object;
 Sensor->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Sensor->SetCollisionResponseToAllChannels(ECR_Overlap);
 Sensor->SetGenerateOverlapEvents(true);
}
void APortalPressureButton::BeginPlay() { Super::BeginPlay(); OnConstruction(GetActorTransform()); }
void APortalPressureButton::OnConstruction(const FTransform& Transform) {
 Super::OnConstruction(Transform);
 Plate->SetRelativeScale3D(FVector(Radius/50,Radius/50,Thickness/100)); Plate->SetMaterial(0,ReleasedMaterial);
 Sensor->SetRelativeLocation(FVector(0,0,Thickness*.5f+3)); Sensor->SetBoxExtent(FVector(Radius,Radius,4));
}
void APortalPressureButton::Tick(float Dt) {
 Super::Tick(Dt); float Mass=0;
 // Query physics bodies directly: simulating meshes need not enable overlap events.
 TArray<FOverlapResult> Hits; FCollisionQueryParams Query(SCENE_QUERY_STAT(PressureButton),false,this);
 FCollisionObjectQueryParams Objects; Objects.AddObjectTypesToQuery(ECC_PhysicsBody); Objects.AddObjectTypesToQuery(ECC_Pawn); Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
 GetWorld()->OverlapMultiByObjectType(Hits,Sensor->GetComponentLocation(),Sensor->GetComponentQuat(),Objects,FCollisionShape::MakeBox(Sensor->GetScaledBoxExtent()),Query);
 TSet<AActor*> Seen;
 for(const auto& Hit:Hits) {
  AActor* Actor=Hit.GetActor(); auto* Body=Hit.GetComponent();
  if(!IsValid(Actor)||!Body||Seen.Contains(Actor)||(!RequiredActorTag.IsNone()&&!Actor->ActorHasTag(RequiredActorTag))) continue;
  Seen.Add(Actor);
  const FVector Local=GetActorTransform().InverseTransformPosition(Actor->GetActorLocation());
  if(FVector2D(Local.X,Local.Y).Size()>Radius*.7f) continue;
  if(bAcceptPlayer && Cast<APawn>(Actor)) { Mass+=FMath::Max(MinimumMass,1.f); continue; }
  if(auto* Carryable=Cast<APortalCarryable>(Actor); Carryable && Carryable->Holder) continue;
  if(bAcceptPhysicsObjects && Body->IsSimulatingPhysics()) Mass+=Body->GetMass();
 }
 SetActive(Mass>0 && Mass>=MinimumMass);
 auto* Desired=bActive?PressedMaterial.Get():ReleasedMaterial.Get();
 if(Plate->GetMaterial(0)!=Desired) Plate->SetMaterial(0,Desired);
}
void APortalPressureButton::ResetGameplay_Implementation() { Super::ResetGameplay_Implementation(); Plate->SetMaterial(0,bActive?PressedMaterial:ReleasedMaterial); }

APortalDoor::APortalDoor() {
 PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.TickGroup=TG_PostUpdateWork;
 RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
 Panel=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Panel")); Panel->SetupAttachment(RootComponent);
 Input=CreateDefaultSubobject<UPortalSignalInput>(TEXT("Input"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cube.Cube"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> Orange(TEXT("/Game/Materials/M_Orange.M_Orange"));
 Panel->SetStaticMesh(Shape.Object); Panel->SetMaterial(0,Orange.Object); Panel->SetCollisionProfileName(TEXT("BlockAll"));
}
void APortalDoor::OnConstruction(const FTransform& Transform) { Super::OnConstruction(Transform); Panel->SetRelativeScale3D(PanelSize/100); }
void APortalDoor::BeginPlay() { Super::BeginPlay(); OnConstruction(GetActorTransform()); ResetGameplay_Implementation(); }
bool APortalDoor::IsObstructed() const {
 TArray<FOverlapResult> Hits; FCollisionQueryParams Query(SCENE_QUERY_STAT(DoorSafety),false,this);
 FCollisionObjectQueryParams Objects; Objects.AddObjectTypesToQuery(ECC_PhysicsBody); Objects.AddObjectTypesToQuery(ECC_Pawn); Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
 GetWorld()->OverlapMultiByObjectType(Hits,GetActorLocation(),GetActorQuat(),Objects,FCollisionShape::MakeBox(PanelSize*GetActorScale3D()*.5+FVector(5)),Query);
 for(const auto& Hit:Hits) if(Cast<APawn>(Hit.GetActor()) || Cast<APortalCarryable>(Hit.GetActor()) || (Hit.GetComponent()&&Hit.GetComponent()->IsSimulatingPhysics())) return true;
 return false;
}
void APortalDoor::Tick(float Dt) {
 Super::Tick(Dt); const bool Active=Input->Evaluate(); if(bLatchOpen&&Active) bLatched=true;
 bool Open=Active||bLatched;
 if(!Open && OpenAmount>0 && IsObstructed()) return;
 OpenAmount=FMath::FInterpConstantTo(OpenAmount,Open?1.f:0.f,Dt,1/FMath::Max(TravelSeconds,.01f));
 Panel->SetRelativeLocation(OpenOffset*OpenAmount);
}
void APortalDoor::ResetGameplay_Implementation() { bLatched=false; OpenAmount=0; Panel->SetRelativeLocation(FVector::ZeroVector); }

APortalLevelExit::APortalLevelExit() {
 PrimaryActorTick.bCanEverTick=true;
 Trigger=CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger")); RootComponent=Trigger;
 Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Trigger->SetCollisionResponseToAllChannels(ECR_Ignore); Trigger->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);
 Input=CreateDefaultSubobject<UPortalSignalInput>(TEXT("Input")); Input->bValueWithoutSources=true;
 ObjectiveText=FText::FromString(TEXT("Solve the chamber and reach the exit."));
 CompleteText=FText::FromString(TEXT("TEST COMPLETE! R: restart chamber"));
}
void APortalLevelExit::OnConstruction(const FTransform& Transform) { Super::OnConstruction(Transform); Trigger->SetBoxExtent(HalfExtent); }
void APortalLevelExit::BeginPlay() { Super::BeginPlay(); OnConstruction(GetActorTransform()); }
void APortalLevelExit::Tick(float Dt) {
 Super::Tick(Dt); if(bComplete || !Input->Evaluate()) return;
 TArray<AActor*> Actors; Trigger->GetOverlappingActors(Actors,APawn::StaticClass());
 for(auto* Actor:Actors) if(auto* Pawn=Cast<APawn>(Actor); Pawn&&Pawn->IsPlayerControlled()) {
  bComplete=true; OnCompleted.Broadcast(); if(bTravelOnCompletion) TravelToNextLevel(); break;
 }
}
bool APortalLevelExit::TravelToNextLevel() {
 if(!bComplete || NextLevel.IsNull()) return false;
 UGameplayStatics::OpenLevelBySoftObjectPtr(this,NextLevel); return true;
}
void APortalLevelExit::ResetGameplay_Implementation() { bComplete=false; }
void UPortalLevelSubsystem::ResetLevelObjects() {
 // Order is intentional: release/reset bodies before sensors and moving mechanisms.
 for(TActorIterator<APortalCarryable> It(GetWorld());It;++It) IPortalResettable::Execute_ResetGameplay(*It);
 for(TActorIterator<AActor> It(GetWorld());It;++It) if(!Cast<APortalCarryable>(*It)&&It->Implements<UPortalResettable>()) IPortalResettable::Execute_ResetGameplay(*It);
}
FText UPortalLevelSubsystem::GetObjective() const {
 for(TActorIterator<APortalLevelExit> It(GetWorld());It;++It) if(It->bComplete) return It->CompleteText;
 for(TActorIterator<APortalLevelExit> It(GetWorld());It;++It) return It->ObjectiveText;
 return FText::GetEmpty();
}
