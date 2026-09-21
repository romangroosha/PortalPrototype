#pragma once
#include "CoreMinimal.h"

// Portal local +X is outward normal, +Y right, +Z up. Scale never changes momentum.
namespace PortalMath {
inline bool CapsuleFitsCircle(const FVector& Local,const FVector& LocalUp,float Radius,float HalfHeight,float PortalRadius) {
 const FVector Segment=LocalUp*FMath::Max(0.f,HalfHeight-Radius);
 const FVector2D A(Local.Y+Segment.Y,Local.Z+Segment.Z),B(Local.Y-Segment.Y,Local.Z-Segment.Z);
 return FMath::Max(A.Size(),B.Size())+Radius<=PortalRadius+.01f;
}
inline float CapsuleSupport(const FVector& Axis,const FVector& CapsuleUp,float Radius,float HalfHeight) {
 return Radius+FMath::Max(0.f,HalfHeight-Radius)*FMath::Abs(FVector::DotProduct(Axis,CapsuleUp));
}
inline bool SphereInView(const FVector& Center, float Radius, const FVector& Eye,
 const FQuat& ViewRotation, float HorizontalFOV, float Aspect) {
 const FVector Local = ViewRotation.UnrotateVector(Center-Eye);
 if (Local.X + Radius <= 0) return false;
 const double TanH = FMath::Tan(FMath::DegreesToRadians(HorizontalFOV*.5f));
 return FMath::Abs(Local.Y)-Radius <= (Local.X+Radius)*TanH &&
        FMath::Abs(Local.Z)-Radius <= (Local.X+Radius)*TanH/FMath::Max(Aspect,.1f);
}
inline FQuat Rotation(const FTransform& From, const FTransform& To) {
 return To.GetRotation() * FQuat(FVector::UpVector, PI) * From.GetRotation().Inverse();
}
inline FVector Point(const FVector& P, const FTransform& From, const FTransform& To) {
 return To.GetLocation() + Rotation(From, To).RotateVector(P - From.GetLocation());
}
inline FVector Vector(const FVector& V, const FTransform& From, const FTransform& To) {
 return Rotation(From, To).RotateVector(V);
}
inline bool Crossing(const FVector& Previous, const FVector& Current, float HalfWidth,
 float HalfHeight, float Radius, float HalfCapsule, FVector& Intersection) {
 if (Previous.X <= 0 || Current.X > 0) return false;
 const double T = Previous.X / (Previous.X - Current.X);
 Intersection = FMath::Lerp(Previous, Current, T);
 return FMath::Abs(Intersection.Y) + Radius <= HalfWidth &&
        FMath::Abs(Intersection.Z) + HalfCapsule <= HalfHeight;
}
}
