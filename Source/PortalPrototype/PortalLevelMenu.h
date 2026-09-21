#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "PortalLevelMenu.generated.h"

UCLASS()
class PORTALPROTOTYPE_API APortalLevelMenuController : public APlayerController
{
 GENERATED_BODY()
public:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
 TSharedPtr<class SWidget> MenuWidget;
};

UCLASS()
class PORTALPROTOTYPE_API APortalLevelMenuGameMode : public AGameModeBase
{
 GENERATED_BODY()
public:
 APortalLevelMenuGameMode();
};
