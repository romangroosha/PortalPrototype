#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "PortalLab.generated.h"

class UCameraComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;
class UPortalGunComponent;
class APortalCube;
class APortalCarryable;

UCLASS()
class PORTALPROTOTYPE_API APortalGate : public AActor {
 GENERATED_BODY()
public:
 APortalGate();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Portal") TObjectPtr<APortalGate> Linked;
 UPROPERTY(EditAnywhere, Category="Portal") float ResolutionScale = 0.75f;
 UPROPERTY(EditAnywhere, Category="Portal") int32 MaxResolution = 1600;
 UPROPERTY(EditAnywhere, Category="Portal", meta=(ClampMin="1", ClampMax="4")) int32 RecursionDepth = 3;
 UPROPERTY(EditAnywhere, Category="Portal", meta=(ClampMin="0.25", ClampMax="1.0")) float NestedResolutionScale = 0.5f;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Surface;
 UPROPERTY(VisibleAnywhere) TObjectPtr<USceneCaptureComponent2D> Capture;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Portal",meta=(ClampMin="10")) float HalfWidth=90;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Portal",meta=(ClampMin="10")) float HalfHeight=140;
 virtual void OnConstruction(const FTransform& Transform) override;
 void UpdateFrameGeometry();
 bool bCircular=false;
 bool ContainsBox(const FVector& Local,float YExtent=0,float ZExtent=0) const;
 bool ContainsCapsule(const FVector& Local,float Radius,float CapsuleHalfHeight) const;
 void UpdateSurfaceGeometry(float Thickness,float Front=0);
 uint64 LastCaptureFrame = MAX_uint64;
 int32 LastRenderPasses = 0;
 void ConfigureGunPortal(bool bOrange);
 void SetGunEnabled(bool bPlaced, bool bLinked);
private:
 UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> GunFrames;
 UPROPERTY() TObjectPtr<UMaterialInterface> GunColour;
 UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> Target;
 UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Material;
 UPROPERTY(Transient) TArray<TObjectPtr<UTextureRenderTarget2D>> NestedTargets;
 UPROPERTY(Transient) TArray<TObjectPtr<USceneCaptureComponent2D>> NestedCaptures;
 UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> TerminalTarget;
};

UCLASS()
class PORTALPROTOTYPE_API APortalPawn : public ACharacter {
 GENERATED_BODY()
public:
 APortalPawn();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UPortalGunComponent> Gun;
 void RegisterPortal(APortalGate* Portal) { Gates.AddUnique(Portal); }
 void ResetPortalTracking(bool bResetView=false);
 void UpdatePortalVisuals();
 void Interact();
 FVector GetGunMuzzleLocation() const;
 UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Interaction") TObjectPtr<APortalCarryable> HeldObject;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Interaction",meta=(ClampMin="50")) float PickupRange=240;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Interaction",meta=(ClampMin="0",ClampMax="32")) int32 InteractionPortalHops=8;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Portal",meta=(ClampMin="0.1")) float CameraRecoverySpeed=8;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Interaction",meta=(ClampMin="1")) float MaximumCarryMass=100;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Lifecycle",meta=(ClampMin="100")) float RespawnDrop=2000;
private:
 UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> TransitGun;
 FVector StartPosition;
 FVector PortalCameraOffset=FVector::ZeroVector;
 FVector DefaultCameraOffset=FVector(0,0,64);
 void RefreshPortalCamera();
 FRotator StartRotation;
 UPROPERTY() TObjectPtr<class UAnimSequence> IdleAnimation;
 UPROPERTY() TObjectPtr<class UAnimSequence> WalkAnimation;
 bool bWalkAnimation=false;
 void UpdateHeldObject();
 void FireBlue();
 void FireOrange();
 void ClearPortals();
 void RemoveBlue();
 void RemoveOrange();
 UPROPERTY() TObjectPtr<UStaticMeshComponent> GunBody;
 UPROPERTY() TObjectPtr<UStaticMeshComponent> GunBarrel;
 void Forward(float Value);
 void Right(float Value);
 void Reset();
 FVector PreviousPosition;
 TArray<TWeakObjectPtr<APortalGate>> Gates;
 bool bSmokeTest = false;
 float SmokeSeconds = 0;
 int32 SmokeTeleports = 0;
};

UCLASS()
class PORTALPROTOTYPE_API APortalLabGameMode : public AGameModeBase {
 GENERATED_BODY()
public:
 APortalLabGameMode();
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Portal rules") bool bEnablePortalGun=true;
 virtual void BeginPlay() override;
};
