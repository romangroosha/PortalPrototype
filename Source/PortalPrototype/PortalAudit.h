#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalAudit.generated.h"
class ACameraActor;
class APortalGate;
class APortalPawn;

// Opt-in, reproducible graphics/performance audit. Does not modify saved assets.
UCLASS()
class APortalAudit : public AActor {
 GENERATED_BODY()
public:
 APortalAudit();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
private:
 void SetVisualCase(int32 Index);
 void SetBenchmark(int32 Index);
 void FinishBenchmark();
 UPROPERTY() TObjectPtr<ACameraActor> Camera;
 UPROPERTY() TObjectPtr<APortalPawn> Pawn;
 UPROPERTY() TObjectPtr<APortalGate> Entry;
 TArray<TWeakObjectPtr<APortalGate>> Gates;
 TArray<TWeakObjectPtr<AActor>> ExitWall;
 FString Directory;
 FString SamplesCSV = TEXT("scenario,frame_ms,gpu_ms,camera_error_cm,captures,crossed\n");
 double StartTime = 0, PreviousTime = 0;
 int32 Stage = -1;
 int32 Frames = 0;
 bool bScreenshot = false;
 TArray<double> FrameTimes;
 FVector PreviousPawnPosition;
 double MaxCameraError = 0;
 int32 Traversals = 0;
};
