#include "PortalMath.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPortalMathTest, "PortalPrototype.Math.TransformAndCrossing",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPortalMathTest::RunTest(const FString& Parameters) {
 const FTransform A(FRotator(0, 33, 0), FVector(125, -82, 140));
 const FTransform B(FRotator(90, -51, 0), FVector(-600, 50, 900));
 const FVector P(52, -77, 22), V(1500, 125, -2300);
 TestTrue(TEXT("Point round trip"), PortalMath::Point(PortalMath::Point(P,A,B),B,A).Equals(P,.001));
 TestTrue(TEXT("Velocity round trip"), PortalMath::Vector(PortalMath::Vector(V,A,B),B,A).Equals(V,.001));
 TestTrue(TEXT("Speed conserved"), FMath::IsNearlyEqual(PortalMath::Vector(V,A,B).Size(),V.Size(),.001));
 TestTrue(TEXT("Plane centers map"), PortalMath::Point(A.GetLocation(),A,B).Equals(B.GetLocation(),.001));
 TestTrue(TEXT("Incoming normal becomes outgoing normal"),
   PortalMath::Vector(-A.GetUnitAxis(EAxis::X),A,B).Equals(B.GetUnitAxis(EAxis::X),.001));
 FVector Hit;
 TestTrue(TEXT("Round portal admits centered upright capsule"),PortalMath::CapsuleFitsCircle(FVector::ZeroVector,FVector::UpVector,30,88,140));
 TestTrue(TEXT("Round portal admits wider side entry"),PortalMath::CapsuleFitsCircle(FVector(0,80,0),FVector::UpVector,30,88,140));
 TestFalse(TEXT("Round portal rejects corner outside circle"),PortalMath::CapsuleFitsCircle(FVector(0,95,30),FVector::UpVector,30,88,140));
 TestTrue(TEXT("Floor aperture uses projected capsule disc"),PortalMath::CapsuleFitsCircle(FVector(0,100,0),FVector::ForwardVector,30,88,140));
 TestTrue(TEXT("High speed crossing"), PortalMath::Crossing(FVector(500,0,0), FVector(-900,0,0),90,140,30,88,Hit));
 TestFalse(TEXT("Back to front rejected"), PortalMath::Crossing(FVector(-1,0,0),FVector(1,0,0),90,140,30,88,Hit));
 TestFalse(TEXT("Capsule clips frame"), PortalMath::Crossing(FVector(5,70,0),FVector(-5,70,0),90,140,30,88,Hit));
 TestFalse(TEXT("No repeated teleport at plane"), PortalMath::Crossing(FVector(0,0,0),FVector(-5,0,0),90,140,30,88,Hit));
 TestTrue(TEXT("Segment intersection, not endpoint bounds"),
   PortalMath::Crossing(FVector(10,0,0),FVector(-90,200,0),90,140,30,88,Hit));
 TestTrue(TEXT("Visible recursive portal"), PortalMath::SphereInView(FVector(600,0,0),170,FVector::ZeroVector,FQuat::Identity,90,16.f/9));
 TestFalse(TEXT("Recursive portal behind camera"), PortalMath::SphereInView(FVector(-600,0,0),170,FVector::ZeroVector,FQuat::Identity,90,16.f/9));
 TestFalse(TEXT("Recursive portal outside field of view"), PortalMath::SphereInView(FVector(600,2000,0),170,FVector::ZeroVector,FQuat::Identity,90,16.f/9));
 TestTrue(TEXT("Near-plane intersection conservatively retained"), PortalMath::SphereInView(FVector(-1,0,0),170,FVector::ZeroVector,FQuat::Identity,90,16.f/9));
 return true;
}
#endif
