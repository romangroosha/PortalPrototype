#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalPuzzleAudit.generated.h"
UCLASS()
class APortalPuzzleAudit : public AActor {
 GENERATED_BODY()
public:
 APortalPuzzleAudit();
 virtual void Tick(float Dt) override;
private:
 float Time=0;
 int Stage=0, Failures=0;
 bool bShot=false;
 int32 DistanceStep=0;
 int32 DropCase=0;
 float PeakUpSpeed=0;
 bool bLostButton=false;
 void Check(bool Value,const TCHAR* Name);
 void Shot(const TCHAR* Name);
};
