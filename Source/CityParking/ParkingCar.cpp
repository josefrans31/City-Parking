#include "ParkingCar.h"

#include "CityShapes.h"
#include "ParkingGameMode.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "UObject/ConstructorHelpers.h"

AParkingCar::AParkingCar()
{
	PrimaryActorTick.bCanEverTick = true;

	// The car turns itself; it must not follow the controller's rotation.
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	Body = CreateDefaultSubobject<UBoxComponent>(TEXT("Body"));
	Body->SetBoxExtent(FVector(CarLength * 0.5f, CarWidth * 0.5f, 70.f));
	Body->SetCollisionObjectType(ECC_Pawn);
	Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Body->SetCollisionResponseToAllChannels(ECR_Block);
	Body->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap); // garbage bags
	Body->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Body->SetGenerateOverlapEvents(true);
	RootComponent = Body;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	auto MakePart = [this](const TCHAR* Name, UStaticMesh* Mesh, const FVector& Loc, const FVector& Size, const FRotator& Rot)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Body);
		C->SetStaticMesh(Mesh);
		C->SetRelativeLocationAndRotation(Loc, Rot);
		C->SetRelativeScale3D(Size / 100.f);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return C;
	};

	BodyMesh = MakePart(TEXT("BodyMesh"), CubeMesh.Object, FVector(0, 0, -15), FVector(CarLength, CarWidth, 70), FRotator::ZeroRotator);
	CabinMesh = MakePart(TEXT("CabinMesh"), CubeMesh.Object, FVector(-30, 0, 45), FVector(240, 170, 55), FRotator::ZeroRotator);

	const FRotator WheelRot(0.f, 0.f, 90.f);
	Wheels.Add(MakePart(TEXT("WheelFL"), CylinderMesh.Object, FVector( 145, -88, -45), FVector(70, 70, 24), WheelRot));
	Wheels.Add(MakePart(TEXT("WheelFR"), CylinderMesh.Object, FVector( 145,  88, -45), FVector(70, 70, 24), WheelRot));
	Wheels.Add(MakePart(TEXT("WheelRL"), CylinderMesh.Object, FVector(-145, -88, -45), FVector(70, 70, 24), WheelRot));
	Wheels.Add(MakePart(TEXT("WheelRR"), CylinderMesh.Object, FVector(-145,  88, -45), FVector(70, 70, 24), WheelRot));

	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(Body);
	CameraArm->TargetArmLength = 900.f;
	CameraArm->SetRelativeRotation(FRotator(-22.f, 0.f, 0.f));
	CameraArm->SocketOffset = FVector(0.f, 0.f, 120.f);
	CameraArm->bInheritPitch = false;
	CameraArm->bInheritRoll = false;
	CameraArm->bEnableCameraLag = true;
	CameraArm->CameraLagSpeed = 8.f;
	CameraArm->bEnableCameraRotationLag = true;
	CameraArm->CameraRotationLagSpeed = 6.f;
	CameraArm->bDoCollisionTest = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
	Camera->FieldOfView = 80.f;
}

void AParkingCar::BeginPlay()
{
	Super::BeginPlay();
	FixedZ = GetActorLocation().Z;

	CityShapes::Paint(BodyMesh, PaintColor);
	CityShapes::Paint(CabinMesh, PaintColor * 0.6f);
	for (UStaticMeshComponent* Wheel : Wheels)
	{
		CityShapes::Paint(Wheel, FLinearColor(0.02f, 0.02f, 0.02f));
	}

	Body->OnComponentBeginOverlap.AddDynamic(this, &AParkingCar::OnBodyOverlap);
}

void AParkingCar::BuildInput()
{
	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = Type;
		return Action;
	};

	ThrottleAction = MakeAction(TEXT("IA_Throttle"), EInputActionValueType::Axis1D);
	SteerAction = MakeAction(TEXT("IA_Steer"), EInputActionValueType::Axis1D);
	HandbrakeAction = MakeAction(TEXT("IA_Handbrake"), EInputActionValueType::Boolean);
	CameraAction = MakeAction(TEXT("IA_Camera"), EInputActionValueType::Boolean);
	RestartAction = MakeAction(TEXT("IA_Restart"), EInputActionValueType::Boolean);
	NextAction = MakeAction(TEXT("IA_Next"), EInputActionValueType::Boolean);

	Mapping = NewObject<UInputMappingContext>(this, TEXT("IMC_Driving"));
	auto Map = [this](UInputAction* Action, const FKey& Key, bool bNegate = false)
	{
		FEnhancedActionKeyMapping& KeyMapping = Mapping->MapKey(Action, Key);
		if (bNegate)
		{
			KeyMapping.Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
		}
	};

	// Keyboard
	Map(ThrottleAction, EKeys::W);
	Map(ThrottleAction, EKeys::Up);
	Map(ThrottleAction, EKeys::S, true);
	Map(ThrottleAction, EKeys::Down, true);
	Map(SteerAction, EKeys::D);
	Map(SteerAction, EKeys::Right);
	Map(SteerAction, EKeys::A, true);
	Map(SteerAction, EKeys::Left, true);
	Map(HandbrakeAction, EKeys::SpaceBar);
	Map(CameraAction, EKeys::C);
	Map(RestartAction, EKeys::R);
	Map(NextAction, EKeys::N);

	// Gamepad
	Map(ThrottleAction, EKeys::Gamepad_RightTriggerAxis);
	Map(ThrottleAction, EKeys::Gamepad_LeftTriggerAxis, true);
	Map(SteerAction, EKeys::Gamepad_LeftX);
	Map(HandbrakeAction, EKeys::Gamepad_FaceButton_Bottom);
	Map(CameraAction, EKeys::Gamepad_FaceButton_Top);
	Map(RestartAction, EKeys::Gamepad_Special_Right);
	Map(NextAction, EKeys::Gamepad_FaceButton_Right);
}

void AParkingCar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		UE_LOG(LogTemp, Error, TEXT("City Parking: Enhanced Input is off. Check Config/DefaultInput.ini."));
		return;
	}

	BuildInput();
	Input->BindAction(CameraAction, ETriggerEvent::Started, this, &AParkingCar::ToggleCamera);
	Input->BindAction(RestartAction, ETriggerEvent::Started, this, &AParkingCar::RequestRestart);
	Input->BindAction(NextAction, ETriggerEvent::Started, this, &AParkingCar::RequestNext);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				Subsystem->ClearAllMappings();
				Subsystem->AddMappingContext(Mapping, 0);
			}
		}
	}
}

float AParkingCar::ReadAxis(const UInputAction* Action) const
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !Action)
	{
		return 0.f;
	}
	if (const UEnhancedPlayerInput* PlayerInput = Cast<UEnhancedPlayerInput>(PC->PlayerInput))
	{
		return PlayerInput->GetActionValue(Action).Get<float>();
	}
	return 0.f;
}

void AParkingCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Dt = FMath::Min(DeltaSeconds, 0.05f);
	HitCooldown = FMath::Max(0.f, HitCooldown - Dt);

	const float Throttle = bControlEnabled ? ReadAxis(ThrottleAction) : 0.f;
	const float SteerInput = bControlEnabled ? FMath::Clamp(ReadAxis(SteerAction), -1.f, 1.f) : 0.f;
	const bool bHandbrake = !bControlEnabled || ReadAxis(HandbrakeAction) > 0.5f;

	// Steering eases in, and springs back to centre a bit faster.
	const bool bReturning = FMath::Abs(SteerInput) < FMath::Abs(Steer);
	Steer = FMath::FInterpConstantTo(Steer, SteerInput, Dt, bReturning ? SteerSpeed * 1.6f : SteerSpeed);

	if (bHandbrake)
	{
		Speed = FMath::FInterpConstantTo(Speed, 0.f, Dt, HandbrakeDeceleration);
	}
	else if (FMath::Abs(Throttle) > 0.05f)
	{
		const bool bBraking = (Speed > 10.f && Throttle < 0.f) || (Speed < -10.f && Throttle > 0.f);
		if (bBraking)
		{
			Speed = FMath::FInterpConstantTo(Speed, 0.f, Dt, BrakeDeceleration * FMath::Abs(Throttle));
		}
		else
		{
			Speed = FMath::Clamp(Speed + Throttle * Acceleration * Dt, -MaxReverseSpeed, MaxForwardSpeed);
		}
	}
	else
	{
		Speed = FMath::FInterpConstantTo(Speed, 0.f, Dt, CoastDeceleration);
	}

	Drive(Dt);

	// Front wheels turn with the steering.
	const FRotator FrontWheel(0.f, Steer * MaxSteerAngle, 90.f);
	Wheels[0]->SetRelativeRotation(FrontWheel);
	Wheels[1]->SetRelativeRotation(FrontWheel);
}

void AParkingCar::Drive(float Dt)
{
	UWorld* World = GetWorld();

	// 1. Turn, like a real car: the faster you go, the faster you turn. No turning when stopped.
	const float YawRate = Speed / Wheelbase * FMath::Tan(FMath::DegreesToRadians(Steer * MaxSteerAngle));
	const float DeltaYaw = FMath::RadiansToDegrees(YawRate * Dt);
	if (!FMath::IsNearlyZero(DeltaYaw, 0.001f))
	{
		FRotator NewRotation = GetActorRotation();
		NewRotation.Yaw += DeltaYaw;

		// Only turn if the car would not end up inside something.
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ParkingCarTurn), false, this);
		const FCollisionShape Shape = FCollisionShape::MakeBox(Body->GetScaledBoxExtent() - FVector(2.f));
		if (!World->OverlapBlockingTestByChannel(GetActorLocation(), NewRotation.Quaternion(), ECC_Pawn, Shape, Params))
		{
			SetActorRotation(NewRotation);
		}
		else
		{
			Speed *= 0.5f;
		}
	}

	// 2. Move forward or back, and slide along whatever we hit.
	const FVector Forward = GetActorForwardVector();
	const FVector Delta = Forward * Speed * Dt;
	if (!Delta.IsNearlyZero())
	{
		FHitResult Hit;
		AddActorWorldOffset(Delta, true, &Hit);
		if (Hit.bBlockingHit)
		{
			FVector Normal = Hit.Normal;
			Normal.Z = 0.f;
			Normal = Normal.GetSafeNormal();

			if (Hit.bStartPenetrating)
			{
				AddActorWorldOffset(Normal * (Hit.PenetrationDepth + 1.f));
			}

			const float HeadOn = FMath::Abs(FVector::DotProduct(Forward, Normal));
			ReportHit(Hit, FMath::Abs(Speed) * HeadOn);

			const FVector Slide = FVector::VectorPlaneProject(Delta * (1.f - Hit.Time), Normal);
			AddActorWorldOffset(FVector(Slide.X, Slide.Y, 0.f), true);
			Speed *= (1.f - 0.85f * HeadOn);
		}
	}

	// 3. Stay on the street.
	FVector Location = GetActorLocation();
	if (!FMath::IsNearlyEqual(Location.Z, FixedZ))
	{
		Location.Z = FixedZ;
		SetActorLocation(Location);
	}
}

void AParkingCar::ReportHit(const FHitResult& Hit, float ImpactSpeed)
{
	if (HitCooldown > 0.f || ImpactSpeed < 40.f)
	{
		return; // gentle touches and long scrapes along a wall count once
	}
	HitCooldown = 0.4f;
	if (AParkingGameMode* Game = GetWorld()->GetAuthGameMode<AParkingGameMode>())
	{
		Game->ReportCollision(Hit.GetComponent(), ImpactSpeed);
	}
}

void AParkingCar::OnBodyOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherComp || !OtherComp->ComponentHasTag(TEXT("Garbage")))
	{
		return;
	}

	// Knock the bag away and only count it once.
	OtherComp->ComponentTags.Remove(TEXT("Garbage"));
	if (OtherComp->IsSimulatingPhysics())
	{
		const FVector Push = GetActorForwardVector() * FMath::Abs(Speed) * 0.9f + FVector(0.f, 0.f, 250.f);
		OtherComp->AddImpulse(Push, NAME_None, true);
	}
	Speed *= 0.9f;

	if (AParkingGameMode* Game = GetWorld()->GetAuthGameMode<AParkingGameMode>())
	{
		Game->ReportGarbage();
	}
}

void AParkingCar::ToggleCamera()
{
	bTopView = !bTopView;
	CameraArm->TargetArmLength = bTopView ? 2200.f : 900.f;
	CameraArm->SetRelativeRotation(FRotator(bTopView ? -70.f : -22.f, 0.f, 0.f));
}

void AParkingCar::RequestRestart()
{
	if (AParkingGameMode* Game = GetWorld()->GetAuthGameMode<AParkingGameMode>())
	{
		Game->RestartLevel();
	}
}

void AParkingCar::RequestNext()
{
	if (AParkingGameMode* Game = GetWorld()->GetAuthGameMode<AParkingGameMode>())
	{
		Game->NextLevel();
	}
}
