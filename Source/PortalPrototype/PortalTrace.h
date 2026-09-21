#pragma once
#include "CoreMinimal.h"
#include "CollisionQueryParams.h"

class UWorld;

namespace PortalTrace {
// Range is the sum of physical ray segments; the distance between paired gates is free.
// The hop budget bounds cycles. A ray that exhausts it stops at the next portal.
bool Line(UWorld* World, FHitResult& Hit, FVector Start, FVector Direction, float Range,
 ECollisionChannel Channel, const FCollisionQueryParams& Query, int32 MaxHops=8, int32* OutHops=nullptr);
// Sweep each physical segment and transform the box orientation at every aperture.
// Target and Orientation return the reachable holding pose, including blocked paths.
void MoveBox(UWorld* World,FVector Start,FVector& Target,FQuat& Orientation,const FVector& HalfExtent,
 const FCollisionQueryParams& Query,bool bCanTraverse,int32 MaxHops=8,int32* OutHops=nullptr);
}
