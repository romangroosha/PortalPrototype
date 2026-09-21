#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalTraversalAudit.generated.h"
UCLASS()
class APortalTraversalAudit : public AActor {
 GENERATED_BODY()
public:
 APortalTraversalAudit();
 virtual void Tick(float Dt) override;
private:
 int32 Stage=0,Checks=0,Failures=0;
 int32 ViewCase=0;
 float Time=0;
 FVector PhysicalExit;
 void Check(bool bValue,const TCHAR* Name);
};
