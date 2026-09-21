#include "PortalPuzzle.h"
#include "PortalLab.h"
#include "PortalMath.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

APortalCarryable::APortalCarryable() {
 PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.TickGroup=TG_PostPhysics;
 Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeightedCube")); RootComponent=Mesh;
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cube.Cube"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mat(TEXT("/Game/Materials/M_Blue.M_Blue"));
 Mesh->SetStaticMesh(Shape.Object); Mesh->SetMaterial(0,Mat.Object);
 Mesh->SetWorldScale3D(FVector(.64)); Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
 Mesh->SetSimulatePhysics(true);
 Mesh->SetLinearDamping(1); Mesh->SetAngularDamping(3); Mesh->BodyInstance.bUseCCD=true;
}
void APortalCarryable::BeginPlay() {
 Super::BeginPlay(); Mesh->SetMassOverrideInKg(NAME_None,MassKg,true);
 ContactMaterial=NewObject<UPhysicalMaterial>(this);
 ContactMaterial->Friction=.8f;
 ContactMaterial->Restitution=0;
 ContactMaterial->bOverrideRestitutionCombineMode=true;
 ContactMaterial->RestitutionCombineMode=EFrictionCombineMode::Min;
 Mesh->SetPhysMaterialOverride(ContactMaterial);
 SpawnPosition=PreviousPosition=GetActorLocation(); SpawnTransform=GetActorTransform();
}
void APortalCarryable::SetHeld(APortalPawn* Pawn) {
 Holder=Pawn; Mesh->SetSimulatePhysics(!Pawn);
 if(!Pawn) {
  // Releasing a carried cube must not restore velocity saved before pickup.
  Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
  Mesh->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
 }
 Mesh->SetCollisionResponseToChannel(ECC_Pawn,Pawn?ECR_Ignore:ECR_Block);
 PreviousPosition=GetActorLocation();
}
void APortalCarryable::ResetObject() {
 SetHeld(nullptr); Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector); Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
 SetActorTransform(SpawnTransform,false,nullptr,ETeleportType::TeleportPhysics); PreviousPosition=SpawnPosition;
}
void APortalCarryable::ResetGameplay_Implementation() { ResetObject(); }
bool APortalCarryable::TryInteract_Implementation(APawn* User) {
 auto* Pawn=Cast<APortalPawn>(User);
 if(!Pawn || !bCanCarry || Holder || Pawn->HeldObject || MassKg>Pawn->MaximumCarryMass) return false;
 Pawn->HeldObject=this; SetHeld(Pawn); return true;
}
FVector APortalCarryable::GetCarryHalfExtent() const {
 return Mesh->GetStaticMesh()?Mesh->GetStaticMesh()->GetBoundingBox().GetExtent()*Mesh->GetComponentScale().GetAbs():FVector(32);
}
float APortalCarryable::ExtentAlong(const FVector& Axis) const {
 return FVector::DotProduct(GetCarryHalfExtent(),Mesh->GetComponentQuat().UnrotateVector(Axis).GetAbs());
}
void APortalCarryable::Tick(float Dt) {
 Super::Tick(Dt);
 if(Holder) { PreviousPosition=GetActorLocation(); return; }
 for(TActorIterator<APortalGate> It(GetWorld());It;++It) {
  auto* G=*It; if(!G->Linked || !bCanTraversePortals) continue;
  FVector Hit; const auto A=G->GetActorTransform(), B=G->Linked->GetActorTransform();
  if(!PortalMath::Crossing(A.InverseTransformPositionNoScale(PreviousPosition),A.InverseTransformPositionNoScale(GetActorLocation()),G->HalfWidth,G->HalfHeight,ExtentAlong(G->GetActorRightVector()),ExtentAlong(G->GetActorUpVector()),Hit)) continue;
  if(!G->ContainsBox(Hit,ExtentAlong(G->GetActorRightVector()),ExtentAlong(G->GetActorUpVector()))) continue;
  const auto V=PortalMath::Vector(Mesh->GetPhysicsLinearVelocity(),A,B);
  const auto W=PortalMath::Vector(Mesh->GetPhysicsAngularVelocityInRadians(),A,B);
  SetActorLocationAndRotation(PortalMath::Point(GetActorLocation(),A,B),PortalMath::Rotation(A,B)*GetActorQuat(),false,nullptr,ETeleportType::TeleportPhysics);
  Mesh->SetPhysicsLinearVelocity(V); Mesh->SetPhysicsAngularVelocityInRadians(W); break;
 }
 PreviousPosition=GetActorLocation(); if(PreviousPosition.Z < SpawnPosition.Z-RespawnDrop) ResetObject();
}
