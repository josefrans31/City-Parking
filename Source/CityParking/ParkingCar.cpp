#include "ParkingCar.h"

#include "CityArtSettings.h"
#include "CityPlayerSettings.h"

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

	// A real car model, if one is picked in Project Settings > City Parking Art. The boxes hide,
	// but the collision box stays the same, so the driving does not change.
	if (CityShapes::AddModel(this, Body, UCityArtSettings::Get().PlayerCar, FVector(0.f, 0.f, -RideHeight), 0.f, CarLength))
	{
		BodyMesh->SetVisibility(false);
		CabinMesh->SetVisibility(false);
		for (UStaticMeshComponent* Wheel : Wheels)
		{
			Wheel->SetVisibility(false);
		}
	}

	Body->OnComponentBeginOverlap.AddDynamic(this, &AParkingCar::OnBodyOverlap);
	ApplyPlayerSettings();
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
	SettingsAction = MakeAction(TEXT("IA_Settings"), EInputActionValueType::Boolean);
	MenuUpAction = MakeAction(TEXT("IA_MenuUp"), EInputActionValueType::Boolean);
	MenuDownAction = MakeAction(TEXT("IA_MenuDown"), EInputActionValueType::Boolean);
	MenuLeftAction = MakeAction(TEXT("IA_MenuLeft"), EInputActionValueType::Boolean);
	MenuRightAction = MakeAction(TEXT("IA_MenuRight"), EInputActionValueType::Boolean);

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
	Map(SettingsAction, EKeys::Tab);
	Map(MenuUpAction, EKeys::Up);
	Map(MenuUpAction, EKeys::W);
	Map(MenuDownAction, EKeys::Down);
	Map(MenuDownAction, EKeys::S);
	Map(MenuLeftAction, EKeys::Left);
	Map(MenuLeftAction, EKeys::A);
	Map(MenuRightAction, EKeys::Right);
	Map(MenuRightAction, EKeys::D);

	// Gamepad
	Map(ThrottleAction, EKeys::Gamepad_RightTriggerAxis);
	Map(ThrottleAction, EKeys::Gamepad_LeftTriggerAxis, true);
	Map(SteerAction, EKeys::Gamepad_LeftX);
	Map(HandbrakeAction, EKeys::Gamepad_FaceButton_Bottom);
	Map(CameraAction, EKeys::Gamepad_FaceButton_Top);
	Map(RestartAction, EKeys::Gamepad_Special_Right);
	Map(NextAction, EKeys::Gamepad_FaceButton_Right);
	Map(SettingsAction, EKeys::Gamepad_Special_Left);
	Map(MenuUpAction, EKeys::Gamepad_DPad_Up);
	Map(MenuDownAction, EKeys::Gamepad_DPad_Down);
	Map(MenuLeftAction, EKeys::Gamepad_DPad_Left);
	Map(MenuRightAction, EKeys::Gamepad_DPad_Right);
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
	Input->BindAction(SettingsAction, ETriggerEvent::Started, this, &AParkingCar::ToggleSettings);
	Input->BindAction(MenuUpAction, ETriggerEvent::Started, this, &AParkingCar::MenuUp);
	Input->BindAction(MenuDownAction, ETriggerEvent::Started, this, &AParkingCar::MenuDown);
	Input->BindAction(MenuLeftAction, ETriggerEvent::Started, this, &AParkingCar::MenuLeft);
	Input->BindAction(MenuRightAction, ETriggerEvent::Started, this, &AParkingCar::MenuRight);

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

	// While the settings menu is open the keys move the menu, and the car holds still.
	const bool bCanDrive = bControlEnabled && !bSettingsOpen;
	const float Throttle = bCanDrive ? ReadAxis(ThrottleAction) : 0.f;
	const float SteerInput = bCanDrive ? FMath::Clamp(ReadAxis(SteerAction), -1.f, 1.f) : 0.f;
	const bool bHandbrake = !bCanDrive || ReadAxis(HandbrakeAction) > 0.5f;

	// Steering eases in, and springs back to centre a bit faster. The wheel also turns slower at speed.
	const bool bReturning = FMath::Abs(SteerInput) < FMath::Abs(Steer);
	const float SpeedFactor = FMath::Clamp(FMath::Abs(Speed) / MaxForwardSpeed, 0.f, 1.f);
	const float WheelRate = SteerSpeed * FMath::Lerp(1.f, 0.45f, SpeedFactor);
	Steer = FMath::FInterpConstantTo(Steer, SteerInput, Dt, bReturning ? WheelRate * 1.8f : WheelRate);

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
	const FRotator FrontWheel(0.f, Steer * CurrentSteerLimit(), 90.f);
	Wheels[0]->SetRelativeRotation(FrontWheel);
	Wheels[1]->SetRelativeRotation(FrontWheel);
}

float AParkingCar::CurrentSteerLimit() const
{
	// Turning radius R = Wheelbase / tan(angle), and side force = speed^2 / R.
	// Keep speed^2 * tan(angle) / Wheelbase under the grip limit.
	const float V2 = FMath::Square(Speed);
	if (V2 < 1.f)
	{
		return MaxSteerAngle;
	}
	const float GripAngle = FMath::RadiansToDegrees(FMath::Atan(MaxSideGrip * Wheelbase / V2));
	return FMath::Min(MaxSteerAngle, GripAngle);
}

void AParkingCar::Drive(float Dt)
{
	UWorld* World = GetWorld();

	// 1. Turn, like a real car: the faster you go, the faster you turn. No turning when stopped.
	// The steering limit shrinks with speed, so fast turns are wide and slow turns can be tight.
	const float YawRate = Speed / Wheelbase * FMath::Tan(FMath::DegreesToRadians(Steer * CurrentSteerLimit()));
	const float DeltaYaw = FMath::RadiansToDegrees(YawRate * Dt);
	if (!FMath::IsNearlyZero(DeltaYaw, 0.001f))
	{
		FRotator NewRotation = GetActorRotation();
		NewRotation.Yaw += DeltaYaw;

		// A real car turns around its rear wheels: the back follows, the nose swings.
		// Keep the rear axle where it is and swing the car around it.
		const FVector HalfWheelbase(Wheelbase * 0.5f, 0.f, 0.f);
		const FVector RearAxle = GetActorLocation() - GetActorRotation().RotateVector(HalfWheelbase);
		const FVector NewLocation = RearAxle + NewRotation.RotateVector(HalfWheelbase);

		// Only turn if the car would not end up inside something.
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ParkingCarTurn), false, this);
		const FCollisionShape Shape = FCollisionShape::MakeBox(Body->GetScaledBoxExtent() - FVector(2.f));
		if (!World->OverlapBlockingTestByChannel(NewLocation, NewRotation.Quaternion(), ECC_Pawn, Shape, Params))
		{
			SetActorLocationAndRotation(NewLocation, NewRotation);
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

void AParkingCar::ApplyPlayerSettings()
{
	UCityPlayerSettings* Settings = UCityPlayerSettings::Get();
	Settings->Clamp();
	MaxSteerAngle = Settings->SteeringAngle;
	SteerSpeed = Settings->SteeringSpeed;
}

void AParkingCar::ToggleSettings()
{
	bSettingsOpen = !bSettingsOpen;
	if (!bSettingsOpen)
	{
		UCityPlayerSettings::Get()->Save();
	}
}

void AParkingCar::MenuUp()
{
	if (bSettingsOpen)
	{
		SettingsRow = (SettingsRow + SettingsRows - 1) % SettingsRows;
	}
}

void AParkingCar::MenuDown()
{
	if (bSettingsOpen)
	{
		SettingsRow = (SettingsRow + 1) % SettingsRows;
	}
}

void AParkingCar::MenuChange(int32 Direction)
{
	if (!bSettingsOpen)
	{
		return;
	}
	UCityPlayerSettings* Settings = UCityPlayerSettings::Get();
	switch (SettingsRow)
	{
	case 0: Settings->SteeringAngle += Direction * 1.f; break;
	case 1: Settings->SteeringSpeed = FMath::RoundToFloat((Settings->SteeringSpeed + Direction * 0.1f) * 10.f) / 10.f; break;
	default: Settings->ResetToDefaults(); break;
	}
	Settings->Clamp();
	ApplyPlayerSettings();
}
