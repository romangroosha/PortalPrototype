#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalMechanisms.h"
#include "PortalPuzzle.generated.h"
class APortalPawn;

UCLASS(Blueprintable)
class PORTALPROTOTYPE_API APortalCarryable : public AActor, public IPortalResettable, public IPortalInteractable {
 GENERATED_BODY()
public:
 APortalCarryable();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Carryable",meta=(ClampMin=".1")) float MassKg=12;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Carryable",meta=(ClampMin="30")) float CarryDistance=125;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Carryable") bool bCanCarry=true;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Carryable") bool bCanTraversePortals=true;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Carryable",meta=(ClampMin="100")) float RespawnDrop=2000;
 UPROPERTY() TObjectPtr<APortalPawn> Holder;
 UPROPERTY(Transient) TObjectPtr<class UPhysicalMaterial> ContactMaterial;
 FVector SpawnPosition, PreviousPosition;
 FTransform SpawnTransform;
 FVector GetCarryHalfExtent() const;
 float ExtentAlong(const FVector& Axis) const;
 virtual void ResetGameplay_Implementation() override;
 virtual bool TryInteract_Implementation(APawn* User) override;
 void ResetObject();
 void SetHeld(APortalPawn* Pawn);
};

// Existing cube assets keep their class; other mesh/shape variants derive from Carryable.
UCLASS(Blueprintable)
class PORTALPROTOTYPE_API APortalCube : public APortalCarryable { GENERATED_BODY() };
