#include "PortalLab.h"
#include "PortalMath.h"
#include "PortalTrace.h"
#include "PortalAudit.h"
#include "PortalGun.h"
#include "PortalGunAudit.h"
#include "PortalPuzzle.h"
#include "PortalPuzzleAudit.h"
#include "PortalFoundationAudit.h"
#include "PortalTraversalAudit.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "GameFramework/PlayerStart.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "SceneView.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UObject/ConstructorHelpers.h"

APortalGate::APortalGate() {
 PrimaryActorTick.bCanEverTick = true;
 PrimaryActorTick.TickGroup = TG_PostUpdateWork; // after player movement and camera update
 RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
 Surface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Surface"));
 Surface->SetupAttachment(RootComponent);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Volume(TEXT("/Engine/BasicShapes/Cube.Cube"));
 Surface->SetStaticMesh(Volume.Object);
 // Front face stays exactly on the mathematical portal plane. The back/side faces
 // cover the near clip plane when the eye approaches it; a single quad would vanish.
 Surface->SetRelativeLocation(FVector(-.05f,0,0));
 Surface->SetRelativeScale3D(FVector(.001f, HalfWidth / 50, HalfHeight / 50));
 Surface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Surface->SetCastShadow(false);
 for(int32 I=0;I<96;++I) {
  auto* Frame=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("GunFrame%d"),I));
  Frame->SetupAttachment(RootComponent); Frame->SetStaticMesh(Volume.Object);
  Frame->SetCollisionEnabled(ECollisionEnabled::NoCollision); Frame->SetCastShadow(false);
  if(I<2) {
   Frame->SetRelativeLocation(FVector(3,I==0?-96:96,0));
   Frame->SetRelativeScale3D(FVector(.06,.12,2.8));
  } else {
   Frame->SetRelativeLocation(FVector(3,0,I==2?-146:146));
   Frame->SetRelativeScale3D(FVector(.06,2.04,.12));
  }
  Frame->SetVisibility(false); GunFrames.Add(Frame);
 }
 Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
 Capture->SetupAttachment(RootComponent);
 Capture->bCaptureEveryFrame = false;
 Capture->bCaptureOnMovement = false;
 Capture->bEnableClipPlane = true;
 Capture->bAlwaysPersistRenderingState = true;
 Capture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
 Capture->ShowFlags.SetMotionBlur(false);
 Capture->ShowFlags.SetTemporalAA(false);
 // Virtual views often lie behind solid floors/walls. Stale occlusion history
 // is invalid for these clipped views and can blank the destination as we move.
 Capture->ShowFlags.SetDisableOcclusionQueries(true);
 Capture->PostProcessSettings.bOverride_AutoExposureMethod = true;
 Capture->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
 Capture->PostProcessSettings.bOverride_AutoExposureBias = true;
 Capture->PostProcessSettings.AutoExposureBias = 0;
 Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
 Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
 Capture->PostProcessSettings.bOverride_BloomIntensity = true;
 Capture->PostProcessSettings.BloomIntensity = 0;
}

void APortalGate::OnConstruction(const FTransform& Transform) {
 Super::OnConstruction(Transform); UpdateFrameGeometry();
 Surface->SetRelativeScale3D(FVector(.001,HalfWidth/50,HalfHeight/50));
}
void APortalGate::UpdateFrameGeometry() {
 for(int I=0;I<GunFrames.Num();++I) {
  auto* Frame=GunFrames[I].Get();
  if(bCircular) {
   const float Angle=2*PI*I/GunFrames.Num();
   Frame->SetRelativeLocation(FVector(3,(HalfWidth+4)*FMath::Cos(Angle),(HalfWidth+4)*FMath::Sin(Angle)));
   Frame->SetRelativeRotation(FRotator(0,0,-FMath::RadiansToDegrees(Angle)));
   Frame->SetRelativeScale3D(FVector(.06,.08,2*(HalfWidth+4)*FMath::Tan(PI/GunFrames.Num())/100+.006));
   continue;
  }
  if(I>=4) { Frame->SetVisibility(false); continue; }
  if(I<2) { Frame->SetRelativeLocation(FVector(3,(I==0?-1:1)*(HalfWidth+6),0)); Frame->SetRelativeScale3D(FVector(.06,.12,HalfHeight/50)); }
  else { Frame->SetRelativeLocation(FVector(3,0,(I==2?-1:1)*(HalfHeight+6))); Frame->SetRelativeScale3D(FVector(.06,(HalfWidth+12)/50,.12)); }
 }
}
void APortalGate::UpdateSurfaceGeometry(float Thickness,float Front) {
 Surface->SetRelativeLocation(FVector(Front-Thickness*.5f,0,0));
 Surface->SetRelativeScale3D(bCircular?FVector(HalfWidth/50,HalfHeight/50,Thickness/100):FVector(Thickness/100,HalfWidth/50,HalfHeight/50));
}
bool APortalGate::ContainsBox(const FVector& Local,float YExtent,float ZExtent) const {
 if(bCircular) return FMath::Square(FMath::Abs(Local.Y)+YExtent)+FMath::Square(FMath::Abs(Local.Z)+ZExtent)<=FMath::Square(HalfWidth);
 return FMath::Abs(Local.Y)+YExtent<=HalfWidth && FMath::Abs(Local.Z)+ZExtent<=HalfHeight;
}
bool APortalGate::ContainsCapsule(const FVector& Local,float Radius,float CapsuleHalfHeight) const {
 if(!bCircular) return ContainsBox(Local,PortalMath::CapsuleSupport(GetActorRightVector(),FVector::UpVector,Radius,CapsuleHalfHeight),PortalMath::CapsuleSupport(GetActorUpVector(),FVector::UpVector,Radius,CapsuleHalfHeight));
 return PortalMath::CapsuleFitsCircle(Local,GetActorQuat().UnrotateVector(FVector::UpVector),Radius,CapsuleHalfHeight,HalfWidth);
}
void APortalGate::BeginPlay() {
 Super::BeginPlay();
 Target = NewObject<UTextureRenderTarget2D>(this);
 Target->RenderTargetFormat = RTF_RGBA16f;
 Target->InitAutoFormat(960, 540);
 Capture->TextureTarget = Target;
 if (auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Portal.M_Portal"))) {
  Material = UMaterialInstanceDynamic::Create(Base, this);
  Material->SetTextureParameterValue(TEXT("PortalTexture"), Target);
  Surface->SetMaterial(0, Material);
 }
 TerminalTarget = NewObject<UTextureRenderTarget2D>(this);
 TerminalTarget->ClearColor = FLinearColor::Black;
 TerminalTarget->RenderTargetFormat = RTF_RGBA16f;
 TerminalTarget->InitAutoFormat(4,4);
 TerminalTarget->UpdateResourceImmediate(true);
 // The paired exit surface lies at the clip plane. Only this entry can recurse.
 for (TActorIterator<APortalGate> It(GetWorld()); It; ++It)
  if (*It != this) Capture->HideComponent(It->Surface);
}

void APortalGate::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);
 auto* PC = UGameplayStatics::GetPlayerController(this, 0);
 if (!Linked || !PC || !Target || !PC->PlayerCameraManager) return;
 Capture->HiddenComponents.Reset();
 Capture->HideComponent(Linked->Surface);
 for(auto Frame:Linked->GunFrames) Capture->HideComponent(Frame);
 FVector Eye; FRotator View;
 PC->GetPlayerViewPoint(Eye, View);
 if(auto* Pawn=Cast<APortalPawn>(PC->GetPawn())) Pawn->UpdatePortalVisuals();
 // Conservative viewport rectangle intersection, including near-plane straddling portals.
 int32 Width, Height; PC->GetViewportSize(Width, Height);
 if (Width <= 0 || Height <= 0) return;
 const float Near = GNearClippingPlane;
 const float TanHalfFOV = FMath::Tan(FMath::DegreesToRadians(PC->PlayerCameraManager->GetFOVAngle()*.5f));
 const float NearCornerRadius = Near * FMath::Sqrt(1 + FMath::Square(TanHalfFOV) +
   FMath::Square(TanHalfFOV * float(Height)/Width));
 const float EyeDistance = FVector::DotProduct(Eye-GetActorLocation(),GetActorForwardVector());
 const float Thickness = FMath::Max(.1f, NearCornerRadius - EyeDistance + .2f);
 UpdateSurfaceGeometry(Thickness);
 // Cull the actual volume, not its zero-thickness face: at grazing floor angles
 // the face can leave the viewport while its near-plane coverage is still visible.
 if (!PortalMath::SphereInView(GetActorLocation(),FMath::Sqrt(HalfWidth*HalfWidth+HalfHeight*HalfHeight+Thickness*Thickness),
     Eye,View.Quaternion(),PC->PlayerCameraManager->GetFOVAngle(),float(Width)/Height)) return;
 const float Scale = FMath::Min(FMath::Clamp(ResolutionScale, .25f, 1.f),
   float(FMath::Max(256, MaxResolution)) / FMath::Max(Width, Height));
 const int32 W = FMath::Max(64, FMath::RoundToInt(Width * Scale));
 const int32 H = FMath::Max(64, FMath::RoundToInt(Height * Scale));
 if (Target->SizeX != W || Target->SizeY != H) Target->ResizeTarget(W, H);
 // Put the plane just in front of the backing wall, so its bounds can be
 // rejected before occlusion, including steep floor-to-wall views.
 Capture->ClipPlaneBase = Linked->GetActorLocation() + Linked->GetActorForwardVector() * .5f;
 Capture->ClipPlaneNormal = Linked->GetActorForwardVector();
 // Geometry wholly behind the exit plane cannot contribute to this view.
 // Exclude it before visibility/occlusion processing as well as clipping pixels:
 // a virtual camera can sit behind the room's solid backing wall at long range.
 for(TActorIterator<AActor> It(GetWorld());It;++It) {
  TInlineComponentArray<UPrimitiveComponent*> Components(*It);
  for(auto* Component:Components) {
   if(!Component->IsRegistered() || !Component->IsVisible()) continue;
   const auto& Bounds=Component->Bounds;
   const FVector N=Capture->ClipPlaneNormal;
   const double Radius=FVector::DotProduct(Bounds.BoxExtent,N.GetAbs());
   if(FVector::DotProduct(Bounds.Origin-Capture->ClipPlaneBase,N)+Radius < -.1)
    Capture->HideComponent(Component);
  }
 }
 Capture->FOVAngle = PC->PlayerCameraManager->GetFOVAngle();
 if (ULocalPlayer* Local = PC->GetLocalPlayer(); Local && Local->ViewportClient) {
  FSceneViewProjectionData Projection;
  if (Local->GetProjectionData(Local->ViewportClient->Viewport, Projection)) {
   Capture->bUseCustomProjectionMatrix = true;
   Capture->CustomProjectionMatrix = Projection.ProjectionMatrix;
  }
 }
 TArray<FVector, TInlineAllocator<4>> Eyes;
 TArray<FQuat, TInlineAllocator<4>> Rotations;
 FVector RecursiveEye = Eye;
 FQuat RecursiveRotation = View.Quaternion();
 const FQuat ThroughRotation = PortalMath::Rotation(GetActorTransform(),Linked->GetActorTransform());
 for (int32 Level=0; Level<FMath::Clamp(RecursionDepth,1,4); ++Level) {
  RecursiveEye = PortalMath::Point(RecursiveEye,GetActorTransform(),Linked->GetActorTransform());
  RecursiveRotation = ThroughRotation * RecursiveRotation;
  Eyes.Add(RecursiveEye); Rotations.Add(RecursiveRotation);
  const bool bEntryFacesEye = FVector::DotProduct(RecursiveEye-GetActorLocation(),GetActorForwardVector()) > 0;
  if (!bEntryFacesEye || !PortalMath::SphereInView(GetActorLocation(),FMath::Sqrt(HalfWidth*HalfWidth+HalfHeight*HalfHeight),RecursiveEye,
    RecursiveRotation,Capture->FOVAngle,float(Width)/Height)) break;
 }
 while (NestedTargets.Num() < Eyes.Num()-1) {
  auto* RT = NewObject<UTextureRenderTarget2D>(this);
  RT->RenderTargetFormat = RTF_RGBA16f;
  RT->InitAutoFormat(64,64);
  NestedTargets.Add(RT);
  // Each recursive camera needs its own view state: sharing one corrupts occlusion
  // history between depths and can make nearby objects disappear from the image.
  auto* NestedCapture = NewObject<USceneCaptureComponent2D>(this);
  NestedCapture->SetupAttachment(RootComponent);
  NestedCapture->bCaptureEveryFrame = false;
  NestedCapture->bCaptureOnMovement = false;
  NestedCapture->bAlwaysPersistRenderingState = true;
  NestedCapture->bEnableClipPlane = true;
  NestedCapture->RegisterComponent();
  NestedCaptures.Add(NestedCapture);
 }
 // Render deepest first into distinct targets; never sample the target being written.
 LastRenderPasses = 0;
 for (int32 Level=Eyes.Num()-1; Level>=0; --Level) {
  UTextureRenderTarget2D* Output = Level==0 ? Target.Get() : NestedTargets[Level-1].Get();
  const float NestedScale = FMath::Pow(FMath::Clamp(NestedResolutionScale,.25f,1.f),Level);
  const int32 LevelW = FMath::Max(64,FMath::RoundToInt(W*NestedScale));
  const int32 LevelH = FMath::Max(64,FMath::RoundToInt(H*NestedScale));
  if (Output->SizeX != LevelW || Output->SizeY != LevelH) Output->ResizeTarget(LevelW,LevelH);
  UTextureRenderTarget2D* Child = Level+1<Eyes.Num() ? NestedTargets[Level].Get() : TerminalTarget.Get();
  if (Material) Material->SetTextureParameterValue(TEXT("PortalTexture"),Child);
  auto* PassCapture = Level==0 ? Capture.Get() : NestedCaptures[Level-1].Get();
  PassCapture->CaptureSource = Capture->CaptureSource;
  PassCapture->ShowFlags = Capture->ShowFlags;
  PassCapture->PostProcessSettings = Capture->PostProcessSettings;
  PassCapture->ClipPlaneBase = Capture->ClipPlaneBase;
  PassCapture->ClipPlaneNormal = Capture->ClipPlaneNormal;
  PassCapture->FOVAngle = Capture->FOVAngle;
  PassCapture->bUseCustomProjectionMatrix = Capture->bUseCustomProjectionMatrix;
  PassCapture->CustomProjectionMatrix = Capture->CustomProjectionMatrix;
  PassCapture->HiddenComponents = Capture->HiddenComponents;
  PassCapture->TextureTarget = Output;
  PassCapture->SetWorldLocationAndRotation(Eyes[Level],Rotations[Level]);
  PassCapture->CaptureScene();
  ++LastRenderPasses;
 }
 Capture->TextureTarget = Target;
 if (Material) Material->SetTextureParameterValue(TEXT("PortalTexture"),Target);
 LastCaptureFrame = GFrameCounter;
}

void APortalGate::ConfigureGunPortal(bool bOrange) {
 bCircular=true;
 HalfWidth=HalfHeight=FMath::Max(HalfWidth,HalfHeight);
 Surface->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
 Surface->SetRelativeRotation(FRotator(90,0,0));
 UpdateSurfaceGeometry(.1f);
 UpdateFrameGeometry();
 GunColour=LoadObject<UMaterialInterface>(nullptr,bOrange?TEXT("/Game/Materials/M_Orange.M_Orange"):TEXT("/Game/Materials/M_Blue.M_Blue"));
 for(auto Frame:GunFrames) { Frame->SetMaterial(0,GunColour); Frame->SetVisibility(true); }
}
void APortalGate::SetGunEnabled(bool bPlaced,bool bLinked) {
 SetActorHiddenInGame(!bPlaced);
 SetActorTickEnabled(bPlaced && bLinked);
 Surface->SetMaterial(0,bLinked?static_cast<UMaterialInterface*>(Material.Get()):GunColour.Get());
 UpdateSurfaceGeometry(.1f,bLinked?0:.55f);
 Capture->bCameraCutThisFrame=true;
 for(auto Nested:NestedCaptures) Nested->bCameraCutThisFrame=true;
}

FVector APortalPawn::GetGunMuzzleLocation() const {
 return GunBarrel->GetComponentLocation()+Camera->GetForwardVector()*12.f;
}
APortalPawn::APortalPawn() {
 PrimaryActorTick.bCanEverTick = true;
 PrimaryActorTick.TickGroup = TG_PostPhysics;
 GetCapsuleComponent()->InitCapsuleSize(30, 88);
 Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
 Camera->SetupAttachment(GetCapsuleComponent());
 Camera->SetRelativeLocation(FVector(0, 0, 64));
 Camera->bUsePawnControlRotation = true;
 Camera->FieldOfView = 90;
 Gun=CreateDefaultSubobject<UPortalGunComponent>(TEXT("PortalGun"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> GunCube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> GunCylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> GunWhite(TEXT("/Game/Materials/M_Wall.M_Wall"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> GunDark(TEXT("/Game/Materials/M_Floor.M_Floor"));
 GunBody=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunBody"));
 GunBody->SetupAttachment(Camera); GunBody->SetStaticMesh(GunCube.Object);
 GunBody->SetRelativeLocation(FVector(52,18,-15)); GunBody->SetRelativeScale3D(FVector(.24,.1,.09));
 GunBody->SetMaterial(0,GunWhite.Object); GunBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 GunBody->SetCastShadow(false);
 GunBarrel=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunBarrel"));
 GunBarrel->SetupAttachment(Camera); GunBarrel->SetStaticMesh(GunCylinder.Object);
 GunBarrel->SetRelativeLocation(FVector(69,18,-15)); GunBarrel->SetRelativeRotation(FRotator(90,0,0));
 GunBarrel->SetRelativeScale3D(FVector(.075,.075,.24));
 GunBarrel->SetMaterial(0,GunDark.Object); GunBarrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 GunBarrel->SetCastShadow(false);
 for(int32 I=0;I<2;++I) {
  auto* Copy=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("TransitGun%d"),I));
  Copy->SetupAttachment(RootComponent); Copy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Copy->SetCastShadow(false); Copy->SetVisibleInSceneCaptureOnly(true); Copy->SetVisibility(false);
  TransitGun.Add(Copy);
 }
 static ConstructorHelpers::FObjectFinder<USkeletalMesh> Manny(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
 GetMesh()->SetSkeletalMesh(Manny.Object);
 GetMesh()->SetRelativeLocationAndRotation(FVector(0,0,-88),FRotator(0,-90,0));
 GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 GetMesh()->SetVisibleInSceneCaptureOnly(true);
 Camera->PostProcessSettings = FPostProcessSettings();
 Camera->PostProcessSettings.bOverride_AutoExposureMethod = true;
 Camera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
 Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
 Camera->PostProcessSettings.AutoExposureBias = 0;
 Camera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
 Camera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
 Camera->PostProcessSettings.bOverride_BloomIntensity = true;
 Camera->PostProcessSettings.BloomIntensity = 0;
 GetCharacterMovement()->MaxWalkSpeed = 600;
 GetCharacterMovement()->InitialPushForceFactor = 100;
 GetCharacterMovement()->PushForceFactor = 150;
 GetCharacterMovement()->JumpZVelocity = 500;
 GetCharacterMovement()->AirControl = .3f;
 GetCharacterMovement()->MaxSimulationTimeStep = 1.f / 120;
 GetCharacterMovement()->MaxSimulationIterations = 16;
}
void APortalPawn::BeginPlay() {
 Super::BeginPlay();
 PrimaryActorTick.AddPrerequisite(GetCharacterMovement(), GetCharacterMovement()->PrimaryComponentTick);
 PreviousPosition = GetActorLocation();
 StartPosition=PreviousPosition; StartRotation=GetActorRotation();
 DefaultCameraOffset=Camera->GetRelativeLocation();
 if(auto* PC=Cast<APlayerController>(Controller); PC&&PC->PlayerCameraManager) {
  // ClampAngle treats an exact 360-degree interval as zero width.
  PC->PlayerCameraManager->ViewRollMin=-179.999f;
  PC->PlayerCameraManager->ViewRollMax=179.999f;
 }
 IdleAnimation=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
 WalkAnimation=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd"));
 GetMesh()->PlayAnimation(IdleAnimation,true);
 for (TActorIterator<APortalGate> It(GetWorld()); It; ++It) Gates.AddUnique(*It);
 GunBody->SetVisibility(Gun->GetPortal(0)!=nullptr); GunBarrel->SetVisibility(Gun->GetPortal(0)!=nullptr);
 bSmokeTest = FParse::Param(FCommandLine::Get(), TEXT("PortalSmokeTest"));
}
void APortalPawn::SetupPlayerInputComponent(UInputComponent* Input) {
 Super::SetupPlayerInputComponent(Input);
 Input->BindAxis(TEXT("Forward"), this, &APortalPawn::Forward);
 Input->BindAxis(TEXT("Right"), this, &APortalPawn::Right);
 Input->BindAxis(TEXT("Turn"), this, &APawn::AddControllerYawInput);
 Input->BindAxis(TEXT("Look"), this, &APawn::AddControllerPitchInput);
 Input->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
 Input->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
 Input->BindAction(TEXT("Reset"), IE_Pressed, this, &APortalPawn::Reset);
 Input->BindAction(TEXT("FireBlue"),IE_Pressed,this,&APortalPawn::FireBlue);
 Input->BindAction(TEXT("FireOrange"),IE_Pressed,this,&APortalPawn::FireOrange);
 Input->BindAction(TEXT("ClearPortals"),IE_Pressed,this,&APortalPawn::ClearPortals);
 Input->BindAction(TEXT("RemoveBlue"),IE_Pressed,this,&APortalPawn::RemoveBlue);
 Input->BindAction(TEXT("RemoveOrange"),IE_Pressed,this,&APortalPawn::RemoveOrange);
 Input->BindAction(TEXT("Interact"),IE_Pressed,this,&APortalPawn::Interact);
}
void APortalPawn::FireBlue() { Gun->Fire(false); }
void APortalPawn::FireOrange() { Gun->Fire(true); }
void APortalPawn::ClearPortals() { Gun->Remove(); }
void APortalPawn::RemoveBlue() { Gun->Remove(0); }
void APortalPawn::RemoveOrange() { Gun->Remove(1); }
void APortalPawn::Forward(float V) { AddMovementInput(FRotator(0, GetControlRotation().Yaw, 0).Vector(), V); }
void APortalPawn::Right(float V) { AddMovementInput(FRotationMatrix(FRotator(0, GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::Y), V); }
void APortalPawn::RefreshPortalCamera() {
 Camera->SetWorldLocation(GetActorLocation()+DefaultCameraOffset+PortalCameraOffset);
 Camera->SetWorldRotation(GetControlRotation());
}
void APortalPawn::ResetPortalTracking(bool bResetView) {
 PreviousPosition=GetActorLocation();
 if(bResetView) { PortalCameraOffset=FVector::ZeroVector; RefreshPortalCamera(); }
}
void APortalPawn::Reset() {
 HeldObject=nullptr;
 GetWorld()->GetSubsystem<UPortalLevelSubsystem>()->ResetLevelObjects();
 SetActorLocation(StartPosition, false, nullptr, ETeleportType::TeleportPhysics);
 Gun->Remove(-1,true);
 GetCharacterMovement()->StopMovementImmediately();
 if (Controller) Controller->SetControlRotation(StartRotation);
 PortalCameraOffset=FVector::ZeroVector; SetActorRotation(FRotator(0,StartRotation.Yaw,0)); RefreshPortalCamera();
 PreviousPosition = GetActorLocation();
}
void APortalPawn::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);
 PortalCameraOffset=FMath::VInterpTo(PortalCameraOffset,FVector::ZeroVector,DeltaSeconds,FMath::Max(.1f,CameraRecoverySpeed));
 if(Controller) {
  FRotator View=Controller->GetControlRotation();
  View.Roll=FMath::FInterpTo(FRotator::NormalizeAxis(View.Roll),0.f,DeltaSeconds,FMath::Max(.1f,CameraRecoverySpeed));
  Controller->SetControlRotation(View);
 }
 RefreshPortalCamera();
 const bool Walking=GetVelocity().Size2D()>10;
 if(Walking!=bWalkAnimation) { bWalkAnimation=Walking; GetMesh()->PlayAnimation(Walking?WalkAnimation:IdleAnimation,true); }
 if (bSmokeTest) {
  const float PreviousSeconds = SmokeSeconds;
  SmokeSeconds += FMath::Min(DeltaSeconds, 1.f / 30);
  if (FMath::FloorToInt(PreviousSeconds) != FMath::FloorToInt(SmokeSeconds))
   UE_LOG(LogTemp, Display, TEXT("SMOKE_TICK %.2f dt=%.4f position=%s velocity=%s"), SmokeSeconds, DeltaSeconds,
    *GetActorLocation().ToString(), *GetVelocity().ToString());
  if (SmokeSeconds > 3 && SmokeSeconds < 8) Forward(1);
  if (PreviousSeconds < 2 && SmokeSeconds >= 2)
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("PortalPreview.png"), false, false);
  if (SmokeSeconds > 9) {
   const bool bPassed = SmokeTeleports >= 2 && GetActorLocation().Z > 0;
   UE_LOG(LogTemp, Display, TEXT("PORTAL_SMOKE %s: %d crossings; position %s"),
    bPassed ? TEXT("PASS") : TEXT("FAIL"), SmokeTeleports, *GetActorLocation().ToString());
   FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
   bSmokeTest = false;
  }
 }
 for (auto Weak : Gates) {
  APortalGate* Gate = Weak.Get();
  if (!Gate || !Gate->Linked) continue;
  const FTransform From = Gate->GetActorTransform(), To = Gate->Linked->GetActorTransform();
  const float Radius=GetCapsuleComponent()->GetScaledCapsuleRadius(),HalfHeight=GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
  auto Extent=[&](const FVector& Axis) { return PortalMath::CapsuleSupport(Axis,FVector::UpVector,Radius,HalfHeight); };
  FVector Hit;
  if (!PortalMath::Crossing(From.InverseTransformPositionNoScale(PreviousPosition),
    From.InverseTransformPositionNoScale(GetActorLocation()), Gate->HalfWidth, Gate->HalfHeight,
    Extent(Gate->GetActorRightVector()),Extent(Gate->GetActorUpVector()), Hit)) continue;
  if(!Gate->ContainsCapsule(Hit,Radius,HalfHeight)) continue;
  const FVector ExitLocal=To.InverseTransformPositionNoScale(PortalMath::Point(From.TransformPositionNoScale(Hit),From,To));
  if(!Gate->Linked->ContainsCapsule(ExitLocal,Radius,HalfHeight)) {
   SetActorLocation(From.TransformPositionNoScale(Hit)+Gate->GetActorForwardVector()*(Extent(Gate->GetActorForwardVector())+.5f),false,nullptr,ETeleportType::TeleportPhysics);
   GetCharacterMovement()->Velocity=FVector::VectorPlaneProject(GetVelocity(),Gate->GetActorForwardVector());
   break;
  }
  const FVector Velocity = PortalMath::Vector(GetVelocity(), From, To);
  if(HeldObject && !HeldObject->bCanTraversePortals) { HeldObject->SetHeld(nullptr); HeldObject=nullptr; }
  const FQuat Rotation = PortalMath::Rotation(From, To);
  const FVector Destination = PortalMath::Point(GetActorLocation(), From, To);
  const FVector Eye=PortalMath::Point(Camera->GetComponentLocation(),From,To);
  const FRotator View=(Rotation*GetControlRotation().Quaternion()).Rotator();
  // Preserve the residual motion after crossing; never push the pawn by a capsule radius.
  SetActorLocationAndRotation(Destination,FRotator(0,View.Yaw,0),false,nullptr,ETeleportType::TeleportPhysics);
  if (Controller) Controller->SetControlRotation(View);
  PortalCameraOffset=Eye-Destination-DefaultCameraOffset;
  GetCharacterMovement()->Velocity = Velocity;
  GetCharacterMovement()->bJustTeleported = true;
  GetCharacterMovement()->SetMovementMode(MOVE_Falling);
  ++SmokeTeleports;
  UE_LOG(LogTemp, Display, TEXT("Portal crossing: speed %.3f -> %.3f"), Velocity.Size(), GetVelocity().Size());
  break;
 }
 PreviousPosition = GetActorLocation();
 RefreshPortalCamera();
 UpdateHeldObject();
 if (PreviousPosition.Z < StartPosition.Z-RespawnDrop) Reset();
}

void APortalPawn::Interact() {
 if(HeldObject) { HeldObject->SetHeld(nullptr); HeldObject=nullptr; return; }
 FVector Eye=Camera->GetComponentLocation(); FHitResult Hit;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(PickupCube),false,this);
 int32 Hops=0;
 if(PortalTrace::Line(GetWorld(),Hit,Eye,GetControlRotation().Vector(),PickupRange,ECC_Visibility,Q,InteractionPortalHops,&Hops)) {
  if(auto* Item=Cast<APortalCarryable>(Hit.GetActor())) {
   if(Hops>0&&!Item->bCanTraversePortals) return;
   // A visible center is insufficient: the carried volume must reach the same
   // point through the same number of apertures before ownership changes.
   FVector Reach=Eye+GetControlRotation().Vector()*Hit.Distance;
   FQuat Facing=FRotator(0,GetControlRotation().Yaw,0).Quaternion();
   auto CarryQuery=Q; CarryQuery.AddIgnoredActor(Item); int32 CarryHops=0;
   PortalTrace::MoveBox(GetWorld(),Eye,Reach,Facing,Item->GetCarryHalfExtent()+FVector(1),CarryQuery,Item->bCanTraversePortals,InteractionPortalHops,&CarryHops);
   if(CarryHops!=Hops || !Reach.Equals(Hit.ImpactPoint,2)) return;
  }
  if(IsValid(Hit.GetActor()) && Hit.GetActor()->Implements<UPortalInteractable>()) IPortalInteractable::Execute_TryInteract(Hit.GetActor(),this);
 }
}
void APortalPawn::UpdateHeldObject() {
 if(!HeldObject || HeldObject->Holder!=this) { HeldObject=nullptr; return; }
 const FVector Eye=Camera->GetComponentLocation();
 FVector Target=Eye+GetControlRotation().Vector()*HeldObject->CarryDistance-FVector(0,0,20);
 FQuat Facing=FRotator(0,GetControlRotation().Yaw,0).Quaternion();
 FCollisionQueryParams Q(SCENE_QUERY_STAT(CarryCube),false,this); Q.AddIgnoredActor(HeldObject);
 PortalTrace::MoveBox(GetWorld(),Eye,Target,Facing,HeldObject->GetCarryHalfExtent()+FVector(1),Q,HeldObject->bCanTraversePortals,InteractionPortalHops);
 HeldObject->SetActorLocationAndRotation(Target,Facing,false,nullptr,ETeleportType::TeleportPhysics);
 HeldObject->PreviousPosition=Target;
}
void APortalPawn::UpdatePortalVisuals() {
 for(auto Copy:TransitGun) Copy->SetVisibility(false);
 UStaticMeshComponent* Originals[]={GunBody.Get(),GunBarrel.Get()};
 for(auto Weak:Gates) {
  auto* G=Weak.Get(); if(!G||!G->Linked) continue;
  const auto A=G->GetActorTransform(), B=G->Linked->GetActorTransform();
  const FVector Eye=A.InverseTransformPositionNoScale(Camera->GetComponentLocation());
  if(Eye.X < 0 || Eye.X>115 || FMath::Abs(Eye.Y)>G->HalfWidth || FMath::Abs(Eye.Z)>G->HalfHeight) continue;
  for(int I=0;I<2;++I) {
   auto* Original=Originals[I]; auto* Copy=TransitGun[I].Get();
   Copy->SetStaticMesh(Original->GetStaticMesh()); Copy->SetMaterial(0,Original->GetMaterial(0));
   Copy->SetWorldTransform(FTransform(PortalMath::Rotation(A,B)*Original->GetComponentQuat(),PortalMath::Point(Original->GetComponentLocation(),A,B),Original->GetComponentScale()));
   Copy->SetVisibility(Original->IsVisible());
  }
  break;
 }
}

APortalLabGameMode::APortalLabGameMode() { DefaultPawnClass = APortalPawn::StaticClass(); HUDClass=APortalGunHUD::StaticClass(); }
void APortalLabGameMode::BeginPlay() {
 Super::BeginPlay();
 if (FParse::Param(FCommandLine::Get(), TEXT("PortalAudit"))) GetWorld()->SpawnActor<APortalAudit>();
 if (FParse::Param(FCommandLine::Get(), TEXT("PortalGunAudit"))) GetWorld()->SpawnActor<APortalGunAudit>();
 if (FParse::Param(FCommandLine::Get(), TEXT("PortalPuzzleAudit"))) GetWorld()->SpawnActor<APortalPuzzleAudit>();
 if (FParse::Param(FCommandLine::Get(), TEXT("PortalFoundationAudit"))) GetWorld()->SpawnActor<APortalFoundationAudit>();
 if (FParse::Param(FCommandLine::Get(), TEXT("PortalTraversalAudit")) || FParse::Param(FCommandLine::Get(),TEXT("PortalTraversalPreview"))) GetWorld()->SpawnActor<APortalTraversalAudit>();
 // Level geometry is authored by Scripts/create_lab.py, including genuine wall openings.
}
