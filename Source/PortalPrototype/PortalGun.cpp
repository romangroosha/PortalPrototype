#include "PortalGun.h"
#include "PortalLab.h"
#include "PortalPuzzle.h"
#include "PortalMath.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/LineBatchComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

APortalWall::APortalWall() {
 RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
 AimSurface=CreateDefaultSubobject<UBoxComponent>(TEXT("AimSurface"));
 AimSurface->SetupAttachment(RootComponent);
 AimSurface->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 AimSurface->SetCollisionResponseToAllChannels(ECR_Ignore);
 AimSurface->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> White(TEXT("/Game/Materials/M_Wall.M_Wall"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> Dark(TEXT("/Game/Materials/M_Floor.M_Floor"));
 PortalableMaterial=White.Object; BlockedMaterial=Dark.Object;
 for (int32 I=0; I<4; ++I) {
  auto* Block=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Block%d"),I));
  Block->SetupAttachment(RootComponent);
  Block->SetStaticMesh(Cube.Object);
  Block->SetMaterial(0,White.Object);
  Block->SetCollisionProfileName(TEXT("BlockAll"));
  Block->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);
  Blocks.Add(Block);
 }
}
void APortalWall::OnConstruction(const FTransform& Transform) { Super::OnConstruction(Transform); Rebuild(); }
void APortalWall::BeginPlay() { Super::BeginPlay(); Rebuild(); }
bool APortalWall::FindPlacement(const FVector& Hit,FVector& Center,FVector2D Aperture) const {
 if (!bPortalable || !GetActorScale3D().Equals(FVector::OneVector,.001)) return false;
 FVector P=GetActorTransform().InverseTransformPositionNoScale(Hit);
 const float YLimit=HalfWidth-Aperture.X-8;
 const float ZMin=-HalfHeight+Aperture.Y;
 const float ZMax=HalfHeight-Aperture.Y-8;
 if (YLimit<0 || ZMax<ZMin || FMath::Abs(P.Y)>HalfWidth+.1f || FMath::Abs(P.Z)>HalfHeight+.1f) return false;
 P.X=0;
 P.Y=FMath::Clamp(P.Y,-YLimit,YLimit);
 P.Z=FMath::Clamp(P.Z,ZMin,ZMax);
 if (GetActorUpVector().Z>.999 && P.Z-ZMin<35) P.Z=ZMin; // floor-aligned upright entrance
 Center=GetActorTransform().TransformPositionNoScale(P);
 return true;
}
void APortalWall::SetOpening(bool bOpen,const FVector& Center,FVector2D Aperture) {
 CircularCenters.Reset();
 bHasOpening=bOpen;
 HoleHalfSize=Aperture;
 HoleCenter=GetActorTransform().InverseTransformPositionNoScale(Center);
 Rebuild();
}
void APortalWall::SetCircularOpenings(const TArray<FVector>& Centers,float Radius) {
 CircularCenters.Reset();
 for(const FVector& Center:Centers) CircularCenters.Add(GetActorTransform().InverseTransformPositionNoScale(Center));
 CircularRadius=Radius; bHasOpening=!CircularCenters.IsEmpty(); Rebuild();
}
void APortalWall::Rebuild() {
 for(auto Block:Blocks) Block->SetMaterial(0,bPortalable?PortalableMaterial:BlockedMaterial);
 AimSurface->SetRelativeLocation(FVector(-Thickness*.5,0,0));
 AimSurface->SetBoxExtent(FVector(Thickness*.5,HalfWidth,HalfHeight));
 auto SetBlock=[&](int I,float Y0,float Y1,float Z0,float Z1) {
  while(Blocks.Num()<=I) {
   auto* Block=NewObject<UStaticMeshComponent>(this);
   Block->SetupAttachment(RootComponent); Block->SetStaticMesh(Blocks[0]->GetStaticMesh());
   Block->SetMaterial(0,bPortalable?PortalableMaterial:BlockedMaterial);
   Block->SetCollisionProfileName(TEXT("BlockAll")); Block->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);
   Block->RegisterComponent(); Blocks.Add(Block);
  }
  const bool Valid=Y1-Y0>.01 && Z1-Z0>.01;
  Blocks[I]->SetVisibility(Valid);
  Blocks[I]->SetCollisionEnabled(Valid?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
  if (Valid) {
   Blocks[I]->SetRelativeLocation(FVector(-Thickness*.5,(Y0+Y1)*.5,(Z0+Z1)*.5));
   Blocks[I]->SetRelativeScale3D(FVector(Thickness/100,(Y1-Y0)/100,(Z1-Z0)/100));
  }
 };
 for(auto Block:Blocks) { Block->SetVisibility(false); Block->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
 if (!CircularCenters.IsEmpty()) {
  // Partition the wall into horizontal bands, subtracting both circular apertures.
  // Use the widest chord in each band so steps never intrude into the disc and
  // catch the capsule at floor level. Exact circular traversal bounds are checked by the gate.
  TArray<float> Cuts={-HalfHeight,HalfHeight};
  for(const FVector& C:CircularCenters) for(int Step=0;Step<=96;++Step)
   Cuts.Add(FMath::Clamp(float(C.Z)-CircularRadius+2*CircularRadius*Step/96,-HalfHeight,HalfHeight));
  Cuts.Sort(); int BlockIndex=0;
  for(int Band=1;Band<Cuts.Num();++Band) {
   const float Z0=Cuts[Band-1],Z1=Cuts[Band]; if(Z1-Z0<.01f) continue;
   TArray<FVector2D> Gaps;
   for(const FVector& C:CircularCenters) {
    const float DZ=(Z0<=C.Z && Z1>=C.Z)?0.f:FMath::Min(FMath::Abs(Z0-float(C.Z)),FMath::Abs(Z1-float(C.Z)));
    if(DZ>=CircularRadius) continue;
    const float Chord=FMath::Sqrt(CircularRadius*CircularRadius-DZ*DZ);
    Gaps.Add(FVector2D(FMath::Max(-HalfWidth,float(C.Y)-Chord),FMath::Min(HalfWidth,float(C.Y)+Chord)));
   }
   Gaps.Sort([](const FVector2D& A,const FVector2D& B){return A.X<B.X;});
   float Cursor=-HalfWidth;
   for(const FVector2D& Gap:Gaps) { if(Gap.X>Cursor) SetBlock(BlockIndex++,Cursor,Gap.X,Z0,Z1); Cursor=FMath::Max(Cursor,float(Gap.Y)); }
   if(Cursor<HalfWidth) SetBlock(BlockIndex++,Cursor,HalfWidth,Z0,Z1);
  }
 } else if (!bHasOpening) {
  SetBlock(0,-HalfWidth,HalfWidth,-HalfHeight,HalfHeight);
  for(int I=1;I<4;++I) SetBlock(I,0,0,0,0);
 } else {
  float L=HoleCenter.Y-HoleHalfSize.X,R=HoleCenter.Y+HoleHalfSize.X;
  float B=HoleCenter.Z-HoleHalfSize.Y,T=HoleCenter.Z+HoleHalfSize.Y;
  SetBlock(0,-HalfWidth,L,-HalfHeight,HalfHeight);
  SetBlock(1,R,HalfWidth,-HalfHeight,HalfHeight);
  SetBlock(2,L,R,-HalfHeight,B);
  SetBlock(3,L,R,T,HalfHeight);
 }
}

UPortalGunComponent::UPortalGunComponent() { PrimaryComponentTick.bCanEverTick=false; }
void UPortalGunComponent::BeginPlay() {
 Super::BeginPlay();
 // Weapon availability is an authored rule, not an accidental side effect of
 // finding some other portal in the map.
 if(!bEnabled) return;
 if(auto* Rules=GetWorld()->GetAuthGameMode<APortalLabGameMode>(); Rules&&!Rules->bEnablePortalGun) return;
 Portals.SetNum(2); Walls.SetNum(2);
 auto* Pawn=Cast<APortalPawn>(GetOwner());
 for(int I=0;I<2;++I) {
  FActorSpawnParameters Params; Params.Owner=GetOwner();
  Portals[I]=GetWorld()->SpawnActor<APortalGate>(APortalGate::StaticClass(),FTransform::Identity,Params);
  Portals[I]->HalfWidth=Portals[I]->HalfHeight=FMath::Max(10.,FMath::Max(PortalHalfSize.X,PortalHalfSize.Y));
  Portals[I]->ConfigureGunPortal(I==1);
  Portals[I]->SetGunEnabled(false,false);
  if(Pawn) Pawn->RegisterPortal(Portals[I]);
 }
}
APortalGate* UPortalGunComponent::GetPortal(int32 Index) const { return Portals.IsValidIndex(Index)?Portals[Index].Get():nullptr; }
bool UPortalGunComponent::CanChange() const {
 for(int I=0;I<2;++I) if(bPlaced[I]) {
  const FVector P=Portals[I]->GetActorTransform().InverseTransformPositionNoScale(GetOwner()->GetActorLocation());
  if(auto* Pawn=Cast<APortalPawn>(GetOwner())) {
   auto Extent=[&](FVector Axis) { return PortalMath::CapsuleSupport(Axis,Pawn->GetActorUpVector(),Pawn->GetCapsuleComponent()->GetScaledCapsuleRadius(),Pawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()); };
   if(FMath::Abs(P.X)<Extent(Portals[I]->GetActorForwardVector())+40 && FMath::Abs(P.Y)<Portals[I]->HalfWidth+Extent(Portals[I]->GetActorRightVector())+5 && FMath::Abs(P.Z)<Portals[I]->HalfHeight+Extent(Portals[I]->GetActorUpVector())+2) return false;
  }
  for(TActorIterator<APortalCarryable> It(GetWorld());It;++It) {
   const FVector C=Portals[I]->GetActorTransform().InverseTransformPositionNoScale(It->GetActorLocation());
   if(FMath::Abs(C.X)<It->ExtentAlong(Portals[I]->GetActorForwardVector())+40 && FMath::Abs(C.Y)<Portals[I]->HalfWidth+It->ExtentAlong(Portals[I]->GetActorRightVector()) && FMath::Abs(C.Z)<Portals[I]->HalfHeight+It->ExtentAlong(Portals[I]->GetActorUpVector())) return false;
  }
 }
 return true;
}
bool UPortalGunComponent::Fire(bool bOrange) {
 auto* Pawn=Cast<APortalPawn>(GetOwner());
 if(!Pawn) return false;
 return FireRay(bOrange,Pawn->Camera->GetComponentLocation(),Pawn->GetControlRotation().Vector());
}
bool UPortalGunComponent::FireRay(bool bOrange,const FVector& Origin,const FVector& Direction) {
 if(!bEnabled || Portals.Num()!=2) return false;
 const int I=bOrange?1:0, Other=1-I;
 FHitResult Hit;
 FCollisionQueryParams Query(SCENE_QUERY_STAT(PortalGun),false,GetOwner());
 const FVector RayEnd=Origin+Direction.GetSafeNormal()*ShotRange;
 const bool bHit=GetWorld()->LineTraceSingleByChannel(Hit,Origin,RayEnd,ECC_Visibility,Query);
 // A shot is visible even when the target rejects placement. Keep its endpoint
 // at the actual hit, independent of any inward adjustment of the portal.
 const auto* Shooter=Cast<APortalPawn>(GetOwner());
 const FVector Muzzle=Shooter?Shooter->GetGunMuzzleLocation():Origin;
 if(auto* Lines=GetWorld()->GetLineBatcher(UWorld::ELineBatcherType::WorldPersistent))
  Lines->DrawLine(Muzzle,bHit?Hit.ImpactPoint:RayEnd,bOrange?FLinearColor(1,.3f,.02f):FLinearColor(.05f,.4f,1),0,2,.12f);
 if(!bHit) {
  Status=TEXT("No surface in range"); return false;
 }
 auto* Wall=Cast<APortalWall>(Hit.GetActor());
 FVector Center;
 const FVector2D Aperture(Portals[I]->HalfWidth,Portals[I]->HalfHeight);
 if(!Wall || FVector::DotProduct(Hit.ImpactNormal,Wall->GetActorForwardVector())<.99 || !Wall->FindPlacement(Hit.ImpactPoint,Center,Aperture)) {
  UE_LOG(LogTemp,Display,TEXT("Portal rejected: actor=%s component=%s point=%s normal=%s"),*GetNameSafe(Hit.GetActor()),*GetNameSafe(Hit.GetComponent()),*Hit.ImpactPoint.ToString(),*Hit.ImpactNormal.ToString());
  Status=TEXT("Aim at a white panel with room for the whole portal"); return false;
 }
 if(bPlaced[Other] && Walls[Other]==Wall && FVector::Dist(Center,Portals[Other]->GetActorLocation())<Aperture.X+Portals[Other]->HalfWidth+16) {
  Status=TEXT("Leave room between the two portal rims"); return false;
 }
 FCollisionQueryParams Clearance(SCENE_QUERY_STAT(PortalClearance),false,GetOwner());
 Clearance.AddIgnoredActor(Wall);
 if(GetWorld()->OverlapBlockingTestByChannel(Center+Wall->GetActorForwardVector()*35,Wall->GetActorQuat(),ECC_Pawn,
   FCollisionShape::MakeBox(FVector(34,Aperture.X-1,Aperture.Y-1)),Clearance)) {
  Status=TEXT("The portal opening is obstructed"); return false;
 }
 if(!CanChange()) { Status=TEXT("Step away from the portal before changing it"); return false; }
 if(Walls[I]) Walls[I]->SetOpening(false);
 Walls[I]=Wall; bPlaced[I]=true;
 Portals[I]->SetActorLocationAndRotation(Center,Wall->GetActorQuat(),false,nullptr,ETeleportType::TeleportPhysics);
 if(auto* Pawn=Cast<APortalPawn>(GetOwner())) Pawn->ResetPortalTracking();
 RefreshPair();
 Status=bPlaced[Other]?TEXT("Portals linked - walk through"):TEXT("Place the other colour to open the passage");
 return true;
}
void UPortalGunComponent::RefreshPair() {
 const bool Linked=bPlaced[0] && bPlaced[1];
 for(int I=0;I<2;++I) {
  Portals[I]->Linked=Linked?Portals[1-I]:nullptr;
  Portals[I]->SetGunEnabled(bPlaced[I],Linked);
 }
 for(int I=0;I<2;++I) if(Walls[I] && (I==0 || Walls[I]!=Walls[0])) {
  TArray<FVector> Centers;
  if(Linked) for(int J=0;J<2;++J) if(Walls[J]==Walls[I]) Centers.Add(Portals[J]->GetActorLocation());
  Walls[I]->SetCircularOpenings(Centers,Portals[I]->HalfWidth);
 }
}
bool UPortalGunComponent::Remove(int32 Index,bool bForce) {
 if(Portals.Num()!=2) return false;
 if(!bForce && !CanChange()) { Status=TEXT("Step away from the portal before removing it"); return false; }
 for(int I=0;I<2;++I) if(Index<0 || I==Index) {
  if(Walls[I]) Walls[I]->SetOpening(false);
  Walls[I]=nullptr; bPlaced[I]=false;
 }
 RefreshPair();
 if(auto* Pawn=Cast<APortalPawn>(GetOwner())) Pawn->ResetPortalTracking();
 Status=TEXT("Portal removed. LMB blue | RMB orange");
 return true;
}
void APortalGunHUD::DrawHUD() {
 Super::DrawHUD();
 auto* Pawn=PlayerOwner?Cast<APortalPawn>(PlayerOwner->GetPawn()):nullptr;
 if(!Canvas || !Pawn || !Pawn->Gun || !Pawn->Gun->GetPortal(0)) return;
 const float X=Canvas->SizeX*.5f,Y=Canvas->SizeY*.5f;
 DrawLine(X-12,Y,X-4,Y,FLinearColor(.05,.35,1),3);
 DrawLine(X+4,Y,X+12,Y,FLinearColor(1,.3,.02),3);
 DrawLine(X,Y-4,X,Y+4,FLinearColor::White,1);
 DrawText(TEXT("LMB: BLUE   RMB: ORANGE   Q/E: REMOVE ONE   X: CLEAR"),FLinearColor::White,24,24,nullptr,1.1);
 DrawText(Pawn->Gun->Status,FLinearColor::White,24,52,nullptr,1.1);
 DrawText(Pawn->Gun->IsPlaced(0)?TEXT("BLUE: PLACED"):TEXT("BLUE: READY"),FLinearColor(.15,.5,1),24,82);
 DrawText(Pawn->Gun->IsPlaced(1)?TEXT("ORANGE: PLACED"):TEXT("ORANGE: READY"),FLinearColor(1,.4,.1),190,82);
 DrawText(GetWorld()->GetSubsystem<UPortalLevelSubsystem>()->GetObjective().ToString(),FLinearColor::White,24,Canvas->SizeY-60,nullptr,1.15);
 if(Pawn->HeldObject) DrawText(TEXT("OBJECT HELD - F to drop"),FLinearColor(.2,.7,1),24,112);
}
