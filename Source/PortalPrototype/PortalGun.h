#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/HUD.h"
#include "PortalGun.generated.h"
class APortalGate;
class UBoxComponent;
class UStaticMeshComponent;

// Oriented panel. Its collision and visible geometry are split around an active aperture.
UCLASS()
class APortalWall : public AActor {
 GENERATED_BODY()
public:
 APortalWall();
 virtual void BeginPlay() override;
 virtual void OnConstruction(const FTransform& Transform) override;
 UPROPERTY(EditAnywhere, Category="Portal wall") float HalfWidth = 800;
 UPROPERTY(EditAnywhere, Category="Portal wall") float HalfHeight = 240;
 UPROPERTY(EditAnywhere, Category="Portal wall") float Thickness = 30;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Portal wall") bool bPortalable=true;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Portal wall") TObjectPtr<UMaterialInterface> PortalableMaterial;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Portal wall") TObjectPtr<UMaterialInterface> BlockedMaterial;
 bool FindPlacement(const FVector& Hit, FVector& Center, FVector2D Aperture=FVector2D(90,140)) const;
 void SetOpening(bool bOpen, const FVector& Center=FVector::ZeroVector, FVector2D Aperture=FVector2D(90,140));
 void SetCircularOpenings(const TArray<FVector>& Centers, float Radius);
 bool HasOpening() const { return bHasOpening; }
private:
 UPROPERTY() TObjectPtr<UBoxComponent> AimSurface;
 UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Blocks;
 bool bHasOpening = false;
 FVector HoleCenter = FVector::ZeroVector;
 FVector2D HoleHalfSize=FVector2D(90,140);
 TArray<FVector> CircularCenters;
 float CircularRadius=140;
 void Rebuild();
};

UCLASS(ClassGroup=(Portal),meta=(BlueprintSpawnableComponent))
class UPortalGunComponent : public UActorComponent {
 GENERATED_BODY()
public:
 UPortalGunComponent();
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Portal gun") FVector2D PortalHalfSize=FVector2D(140,140);
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Portal gun",meta=(ClampMin="100")) float ShotRange=5000;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Portal gun") bool bEnabled=true;
 virtual void BeginPlay() override;
 bool Fire(bool bOrange);
 bool FireRay(bool bOrange, const FVector& Origin, const FVector& Direction);
 bool Remove(int32 Index=-1,bool bForce=false);
 APortalGate* GetPortal(int32 Index) const;
 bool IsPlaced(int32 Index) const { return bPlaced[Index]; }
 FString Status = TEXT("LMB blue | RMB orange | Q/E remove one | X clear");
private:
 UPROPERTY() TArray<TObjectPtr<APortalGate>> Portals;
 UPROPERTY() TArray<TObjectPtr<APortalWall>> Walls;
 bool bPlaced[2] = {false,false};
 bool CanChange() const;
 void RefreshPair();
};

UCLASS()
class APortalGunHUD : public AHUD {
 GENERATED_BODY()
public:
 virtual void DrawHUD() override;
};
