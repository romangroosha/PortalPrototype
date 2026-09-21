#include "PortalLevelMenu.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
class SPortalLevelMenu : public SCompoundWidget
{
public:
 virtual bool SupportsKeyboardFocus() const override { return true; }
 SLATE_BEGIN_ARGS(SPortalLevelMenu) {}
  SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controller)
 SLATE_END_ARGS()

 void Construct(const FArguments& Args)
 {
  Controller = Args._Controller;
  ButtonStyle = FButtonStyle()
   .SetNormal(FSlateColorBrush(FLinearColor(.055f, .055f, .055f)))
   .SetHovered(FSlateColorBrush(FLinearColor(.19f, .19f, .19f)))
   .SetPressed(FSlateColorBrush(FLinearColor(.28f, .28f, .28f)))
   .SetNormalPadding(FMargin(18, 12)).SetPressedPadding(FMargin(18, 12));

  TSharedPtr<SScrollBox> List;
  ChildSlot
  [
   SNew(SBorder)
   .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
   .BorderBackgroundColor(FLinearColor(.025f, .025f, .025f))
   .Padding(32).HAlign(HAlign_Center)
   [
    SNew(SBox).WidthOverride(760)
    [
     SNew(SVerticalBox)
     + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 20)
     [ SNew(STextBlock).Text(FText::FromString(TEXT("Выбор уровня")))
       .Font(FCoreStyle::GetDefaultFontStyle("Regular", 26)) ]
     + SVerticalBox::Slot().FillHeight(1)
     [ SAssignNew(List, SScrollBox).ScrollBarAlwaysVisible(true) ]
    ]
   ]
  ];

  auto& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
  // Discover authored maps, including new subfolders, without imported asset demo maps.
  Registry.ScanPathsSynchronous({TEXT("/Game/Maps")});
  FARFilter Filter;
  Filter.PackagePaths.Add(TEXT("/Game/Maps"));
  Filter.ClassPaths.Add(UWorld::StaticClass()->GetClassPathName());
  Filter.bRecursivePaths = true;
  TArray<FAssetData> Maps;
  Registry.GetAssets(Filter, Maps);
  Maps.Sort([](const FAssetData& A, const FAssetData& B) {
   const int32 NameOrder = A.AssetName.ToString().Compare(B.AssetName.ToString());
   return NameOrder == 0 ? A.PackageName.LexicalLess(B.PackageName) : NameOrder < 0;
  });

  int32 Count = 0;
  for (const FAssetData& Map : Maps)
  {
   if (Map.PackageName == FName(TEXT("/Game/Maps/LevelMenu"))) continue;
   const FName Package = Map.PackageName;
   const bool bDuplicateName = Maps.ContainsByPredicate([&Map](const FAssetData& Other) {
    return Other.AssetName == Map.AssetName && Other.PackageName != Map.PackageName;
   });
   List->AddSlot().Padding(0, 0, 8, 5)
   [
    SNew(SButton).ButtonStyle(&ButtonStyle).Cursor(EMouseCursor::Hand)
    .ToolTipText(FText::FromName(Package))
    .HAlign(HAlign_Fill)
    .IsEnabled_Lambda([this] { return !bOpening; })
    .OnClicked_Lambda([this, Package] {
     if (Controller.IsValid() && !bOpening)
     {
      bOpening = true;
      UE_LOG(LogTemp, Display, TEXT("LEVEL_MENU Open %s"), *Package.ToString());
      UGameplayStatics::OpenLevel(Controller.Get(), Package);
     }
     return FReply::Handled();
    })
    [
     SNew(SVerticalBox)
     + SVerticalBox::Slot().AutoHeight()
     [ SNew(STextBlock).Text(FText::FromName(Map.AssetName))
       .Font(FCoreStyle::GetDefaultFontStyle("Regular", 20)) ]
     + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
     [ SNew(STextBlock).Text(FText::FromName(Package))
       .Visibility(bDuplicateName ? EVisibility::Visible : EVisibility::Collapsed)
       .ColorAndOpacity(FLinearColor(.6f, .6f, .6f))
       .Font(FCoreStyle::GetDefaultFontStyle("Regular", 11)) ]
    ]
   ];
   ++Count;
  }
  if (Count == 0)
   List->AddSlot()[SNew(STextBlock).Text(FText::FromString(TEXT("Уровни не найдены")))];
  UE_LOG(LogTemp, Display, TEXT("LEVEL_MENU Ready: %d maps"), Count);
 }

private:
 FButtonStyle ButtonStyle;
 TWeakObjectPtr<APlayerController> Controller;
 bool bOpening = false;
};
}

void APortalLevelMenuController::BeginPlay()
{
 Super::BeginPlay();
 if (!IsLocalController() || !GetWorld()->GetGameViewport()) return;
 MenuWidget = SNew(SPortalLevelMenu).Controller(this);
 GetWorld()->GetGameViewport()->AddViewportWidgetContent(MenuWidget.ToSharedRef(), 100);
 bShowMouseCursor = true;
 FInputModeUIOnly Input;
 Input.SetWidgetToFocus(MenuWidget);
 Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
 SetInputMode(Input);
}

void APortalLevelMenuController::EndPlay(const EEndPlayReason::Type Reason)
{
 if (MenuWidget.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
  GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(MenuWidget.ToSharedRef());
 MenuWidget.Reset();
 SetInputMode(FInputModeGameOnly());
 bShowMouseCursor = false;
 Super::EndPlay(Reason);
}

APortalLevelMenuGameMode::APortalLevelMenuGameMode()
{
 PlayerControllerClass = APortalLevelMenuController::StaticClass();
 DefaultPawnClass = nullptr;
 HUDClass = nullptr;
}
