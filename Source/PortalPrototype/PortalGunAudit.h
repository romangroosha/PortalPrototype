#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalGunAudit.generated.h"
class APortalPawn;
class APortalWall;
UCLASS()
class APortalGunAudit : public AActor {
 GENERATED_BODY()
public:
 APortalGunAudit();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
private:
 UPROPERTY() TObjectPtr<APortalPawn> Pawn;
 bool Check(bool Value,const TCHAR* Name);
 bool Blocked(APortalWall* Wall,const FVector& Center) const;
 void CheckActions();
 float Seconds=0;
 int32 Stage=0, Failures=0, Checks=0, Crossings=0;
 FVector LastPosition;
 double MaxCameraError=0;
};
