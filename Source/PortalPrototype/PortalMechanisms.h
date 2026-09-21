#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/Interface.h"
#include "PortalMechanisms.generated.h"
class UBoxComponent;
class APawn;

UINTERFACE(BlueprintType)
class UPortalResettable : public UInterface { GENERATED_BODY() };
class IPortalResettable {
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Portal|Lifecycle") void ResetGameplay();
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPortalSignalChanged,bool,bActive);
UINTERFACE(BlueprintType)
class UPortalInteractable : public UInterface { GENERATED_BODY() };
class IPortalInteractable {
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintNativeEvent,BlueprintCallable,Category="Portal|Interaction") bool TryInteract(APawn* User);
};
UCLASS(Blueprintable)
class PORTALPROTOTYPE_API APortalSignal : public AActor, public IPortalResettable {
 GENERATED_BODY()
public:
 APortalSignal();
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Signal") bool bInitiallyActive=false;
 UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Signal") bool bActive=false;
 UPROPERTY(BlueprintAssignable,Category="Signal") FPortalSignalChanged OnSignalChanged;
 UFUNCTION(BlueprintCallable,Category="Signal") void SetActive(bool bValue);
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 virtual void ResetGameplay_Implementation() override;
};

UCLASS(Blueprintable)
class PORTALPROTOTYPE_API APortalTimedButton : public APortalSignal, public IPortalInteractable {
 GENERATED_BODY()
public:
 APortalTimedButton();
 virtual void BeginPlay() override;
 virtual void Tick(float Dt) override;
 virtual bool TryInteract_Implementation(APawn* User) override;
 virtual void ResetGameplay_Implementation() override;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Switch") TObjectPtr<UStaticMeshComponent> SwitchMesh;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Switch",meta=(ClampMin=".1")) float Duration=3;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Switch") bool bToggle=false;
 UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Switch") float Remaining=0;
};

UENUM(BlueprintType)
enum class EPortalSignalRule : uint8 { All, Any };

UCLASS(ClassGroup=(Portal),meta=(BlueprintSpawnableComponent))
class PORTALPROTOTYPE_API UPortalSignalInput : public UActorComponent {
 GENERATED_BODY()
public:
 UPortalSignalInput();
 UPROPERTY(EditInstanceOnly,BlueprintReadWrite,Category="Signal") TArray<TObjectPtr<APortalSignal>> Sources;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Signal") EPortalSignalRule Rule=EPortalSignalRule::All;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Signal") bool bInvert=false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Signal") bool bValueWithoutSources=false;
 UPROPERTY(BlueprintAssignable,Category="Signal") FPortalSignalChanged OnValueChanged;
 UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Signal") bool bValue=false;
 UFUNCTION(BlueprintCallable,Category="Signal") void RefreshBindings();
 UFUNCTION(BlueprintCallable,Category="Signal") bool Evaluate() const;
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
 UPROPERTY() TArray<TObjectPtr<APortalSignal>> BoundSources;
 UFUNCTION() void SourceChanged(bool bIgnored);
};

UCLASS(Blueprintable)
class PORTALPROTOTYPE_API APortalPressureButton : public APortalSignal {
 GENERATED_BODY()
public:
 APortalPressureButton();
 virtual void BeginPlay() override;
 virtual void OnConstruction(const FTransform& Transform) override;
 virtual void Tick(float Dt) override;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Button") TObjectPtr<UStaticMeshComponent> Plate;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Button") TObjectPtr<UBoxComponent> Sensor;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Button",meta=(ClampMin="5")) float Radius=80;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Button",meta=(ClampMin="1")) float Thickness=16;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Button",meta=(ClampMin="0")) float MinimumMass=1;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Button") bool bAcceptPlayer=false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Button") bool bAcceptPhysicsObjects=true;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Button") FName RequiredActorTag;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Button") TObjectPtr<UMaterialInterface> ReleasedMaterial;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Button") TObjectPtr<UMaterialInterface> PressedMaterial;
 virtual void ResetGameplay_Implementation() override;
};

UCLASS(Blueprintable)
class PORTALPROTOTYPE_API APortalDoor : public AActor, public IPortalResettable {
 GENERATED_BODY()
public:
 APortalDoor();
 virtual void OnConstruction(const FTransform& Transform) override;
 virtual void BeginPlay() override;
 virtual void Tick(float Dt) override;
 virtual void ResetGameplay_Implementation() override;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Door") TObjectPtr<UStaticMeshComponent> Panel;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Door") TObjectPtr<UPortalSignalInput> Input;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Door") FVector PanelSize=FVector(35,240,300);
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Door") FVector OpenOffset=FVector(0,0,320);
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Door",meta=(ClampMin=".01")) float TravelSeconds=1;
 UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Door") float OpenAmount=0;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Door") bool bLatchOpen=false;
 bool IsObstructed() const;
private:
 bool bLatched=false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPortalLevelCompleted);
UCLASS(Blueprintable)
class PORTALPROTOTYPE_API APortalLevelExit : public AActor, public IPortalResettable {
 GENERATED_BODY()
public:
 APortalLevelExit();
 virtual void BeginPlay() override;
 virtual void OnConstruction(const FTransform& Transform) override;
 virtual void Tick(float Dt) override;
 virtual void ResetGameplay_Implementation() override;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Exit") TObjectPtr<UBoxComponent> Trigger;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Exit") TObjectPtr<UPortalSignalInput> Input;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Exit") FVector HalfExtent=FVector(130,120,160);
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Exit") FText ObjectiveText;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Exit") FText CompleteText;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Exit") TSoftObjectPtr<UWorld> NextLevel;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Exit") bool bTravelOnCompletion=false;
 UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Exit") bool bComplete=false;
 UPROPERTY(BlueprintAssignable,Category="Exit") FPortalLevelCompleted OnCompleted;
 UFUNCTION(BlueprintCallable,Category="Exit") bool TravelToNextLevel();
};

UCLASS()
class PORTALPROTOTYPE_API UPortalLevelSubsystem : public UWorldSubsystem {
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable,Category="Portal|Lifecycle") void ResetLevelObjects();
 UFUNCTION(BlueprintCallable,Category="Portal|Lifecycle") FText GetObjective() const;
};
