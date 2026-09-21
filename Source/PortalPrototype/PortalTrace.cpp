#include "PortalTrace.h"
#include "PortalLab.h"
#include "PortalMath.h"
#include "EngineUtils.h"
#include "Engine/World.h"

namespace {
APortalGate* FirstCrossing(UWorld* World,const FVector& Start,const FVector& End,FVector& Point,float& Distance) {
 APortalGate* Nearest=nullptr; Distance=FVector::Distance(Start,End);
 for(TActorIterator<APortalGate> It(World);It;++It) {
  auto* Gate=*It;
  if(!IsValid(Gate->Linked) || Gate->IsHidden() || Gate->Linked->IsHidden()) continue;
  const FTransform Transform=Gate->GetActorTransform(); FVector LocalHit;
  if(!PortalMath::Crossing(Transform.InverseTransformPositionNoScale(Start),
     Transform.InverseTransformPositionNoScale(End),Gate->HalfWidth,Gate->HalfHeight,0,0,LocalHit)) continue;
  if(!Gate->ContainsBox(LocalHit)) continue;
  const FVector Candidate=Transform.TransformPositionNoScale(LocalHit);
  const float Along=FVector::Distance(Start,Candidate);
  if(Along<=Distance) { Nearest=Gate; Distance=Along; Point=Candidate; }
 }
 return Nearest;
}
float Support(const FVector& Extent,const FQuat& Rotation,const FVector& Axis) {
 return FVector::DotProduct(Extent,Rotation.UnrotateVector(Axis).GetAbs());
}
bool Fits(APortalGate* Gate,const FVector& Center,const FVector& Extent,const FQuat& Rotation) {
 const FVector Local=Gate->GetActorTransform().InverseTransformPositionNoScale(Center);
 return Gate->ContainsBox(Local,Support(Extent,Rotation,Gate->GetActorRightVector()),Support(Extent,Rotation,Gate->GetActorUpVector()));
}
}

bool PortalTrace::Line(UWorld* World,FHitResult& Hit,FVector Start,FVector Direction,float Range,
 ECollisionChannel Channel,const FCollisionQueryParams& Query,int32 MaxHops,int32* OutHops) {
 Hit=FHitResult();
 if(OutHops) *OutHops=0;
 Direction=Direction.GetSafeNormal();
 if(!World || Direction.IsNearlyZero() || Range<=0) return false;
 float Travelled=0;
 for(int32 Hop=0; ; ++Hop) {
  const FVector End=Start+Direction*Range;
  FHitResult Solid;
  const bool bSolid=World->LineTraceSingleByChannel(Solid,Start,End,Channel,Query);
  FVector Entry; float Distance;
  APortalGate* Nearest=FirstCrossing(World,Start,End,Entry,Distance);
  // Placement aim surfaces coincide with the aperture. A nearer physical obstacle wins.
  if(bSolid && Solid.Distance+.1f<Distance) Nearest=nullptr;
  if(!Nearest) {
   if(bSolid) { Hit=Solid; Hit.Distance+=Travelled; }
   return bSolid;
  }
  if(Hop>=FMath::Max(0,MaxHops)) return false;
  if(OutHops) *OutHops=Hop+1;
  const FTransform From=Nearest->GetActorTransform(),To=Nearest->Linked->GetActorTransform();
  Direction=PortalMath::Vector(Direction,From,To).GetSafeNormal();
  constexpr float ExitEpsilon=.01f;
  Start=PortalMath::Point(Entry,From,To)+Direction*ExitEpsilon;
  Range-=Distance+ExitEpsilon;
  Travelled+=Distance+ExitEpsilon;
  if(Range<=0) return false;
 }
}

void PortalTrace::MoveBox(UWorld* World,FVector Start,FVector& Target,FQuat& Orientation,const FVector& HalfExtent,
 const FCollisionQueryParams& Query,bool bCanTraverse,int32 MaxHops,int32* OutHops) {
 if(OutHops) *OutHops=0;
 if(!World) return;
 const FCollisionShape Shape=FCollisionShape::MakeBox(HalfExtent);
 for(int32 Hop=0; ; ++Hop) {
  FVector Entry; float Distance;
  auto* Gate=FirstCrossing(World,Start,Target,Entry,Distance);
  FVector SegmentEnd=Gate?Entry:Target;
  FQuat ExitOrientation=Orientation; FVector ExitPoint=FVector::ZeroVector;
  bool bPass=false;
  if(Gate) {
   const auto From=Gate->GetActorTransform(),To=Gate->Linked->GetActorTransform();
   ExitOrientation=PortalMath::Rotation(From,To)*Orientation;
   ExitPoint=PortalMath::Point(Entry,From,To);
   bPass=bCanTraverse && Hop<FMath::Max(0,MaxHops) && Fits(Gate,Entry,HalfExtent,Orientation) && Fits(Gate->Linked,ExitPoint,HalfExtent,ExitOrientation);
   if(!bPass) {
    const FVector Direction=(Target-Start).GetSafeNormal();
    const float NormalSpeed=FMath::Max(.0001f,FMath::Abs(FVector::DotProduct(Direction,Gate->GetActorForwardVector())));
    const float Clearance=(Support(HalfExtent,Orientation,Gate->GetActorForwardVector())+1)/NormalSpeed;
    SegmentEnd=Start+Direction*FMath::Max(0.f,Distance-Clearance);
   }
  }
  FHitResult Hit;
  if(World->SweepSingleByChannel(Hit,Start,SegmentEnd,Orientation,ECC_Pawn,Shape,Query)) { Target=Hit.Location; return; }
  if(!Gate || !bPass) { Target=SegmentEnd; return; }
  Target=PortalMath::Point(Target,Gate->GetActorTransform(),Gate->Linked->GetActorTransform());
  Orientation=ExitOrientation;
  Start=ExitPoint+(Target-ExitPoint).GetSafeNormal()*.01f;
  if(OutHops) *OutHops=Hop+1;
  if(FVector::DistSquared(Target,ExitPoint)<.0001f) { Target=ExitPoint; return; }
 }
}
