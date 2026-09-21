#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalFoundationAudit.generated.h"
UCLASS()
class APortalFoundationAudit : public AActor {
 GENERATED_BODY()
public:
 APortalFoundationAudit();
 virtual void Tick(float Dt) override;
private:
 int Stage=0,Failures=0,Checks=0;
 float Time=0;
 void Check(bool Value,const TCHAR* Name);
};
