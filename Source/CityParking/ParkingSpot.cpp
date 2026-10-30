#include "ParkingSpot.h"

#include "CityShapes.h"
#include "ParkingCar.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AParkingSpot::AParkingSpot()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void AParkingSpot::Init(float InLength, float InWidth)
{
	Length = InLength;
	Width = InWidth;

	const FLinearColor Green(0.05f, 0.8f, 0.2f);
	const FLinearColor White(0.9f, 0.9f, 0.9f);
	const FRotator Flat = FRotator::ZeroRotator;

	// Green fill with a white border, painted on the street.
	CityShapes::Add(this, Root, CityShapes::Cube(), FVector(0, 0, 0.6f), FVector(Length, Width, 1.f), Flat, Green * 0.6f, EShapeCollision::None);
	CityShapes::Add(this, Root, CityShapes::Cube(), FVector( Length * 0.5f, 0, 1.2f), FVector(15, Width, 1.f), Flat, White, EShapeCollision::None);
	CityShapes::Add(this, Root, CityShapes::Cube(), FVector(-Length * 0.5f, 0, 1.2f), FVector(15, Width, 1.f), Flat, White, EShapeCollision::None);
	CityShapes::Add(this, Root, CityShapes::Cube(), FVector(0,  Width * 0.5f, 1.2f), FVector(Length, 15, 1.f), Flat, White, EShapeCollision::None);
	CityShapes::Add(this, Root, CityShapes::Cube(), FVector(0, -Width * 0.5f, 1.2f), FVector(Length, 15, 1.f), Flat, White, EShapeCollision::None);

	// Tall beam and a "P" sign block you can spot from down the street.
	Beacon = CityShapes::Add(this, Root, CityShapes::Cylinder(), FVector(0, 0, 1200), FVector(40, 40, 2000), Flat, Green, EShapeCollision::None);
	CityShapes::Add(this, Root, CityShapes::Cube(), FVector(0, 0, 2300), FVector(220, 220, 220), Flat, FLinearColor(0.05f, 0.3f, 0.9f), EShapeCollision::None);
}

void AParkingSpot::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	if (Beacon)
	{
		const float Pulse = 1.f + 0.25f * FMath::Sin(Time * 4.f);
		Beacon->SetRelativeScale3D(FVector(0.4f * Pulse, 0.4f * Pulse, 20.f));
	}
}

AParkingSpot::FCheck AParkingSpot::Check(const AParkingCar* Car) const
{
	FCheck Result;
	if (!Car)
	{
		return Result;
	}

	const FTransform CarTransform = Car->GetActorTransform();
	const FTransform SpotTransform = GetActorTransform();
	const float HalfL = AParkingCar::CarLength * 0.5f;
	const float HalfW = AParkingCar::CarWidth * 0.5f;

	Result.bInside = true;
	for (const FVector& Corner : { FVector(HalfL, HalfW, 0), FVector(HalfL, -HalfW, 0), FVector(-HalfL, HalfW, 0), FVector(-HalfL, -HalfW, 0) })
	{
		const FVector Local = SpotTransform.InverseTransformPosition(CarTransform.TransformPosition(Corner));
		if (FMath::Abs(Local.X) > Length * 0.5f || FMath::Abs(Local.Y) > Width * 0.5f)
		{
			Result.bInside = false;
			break;
		}
	}

	const float Delta = FMath::Abs(FMath::FindDeltaAngleDegrees(Car->GetActorRotation().Yaw, GetActorRotation().Yaw));
	const float AngleError = FMath::Min(Delta, 180.f - Delta); // parked backwards still counts as straight
	Result.bStraight = AngleError < 12.f;
	Result.bFacingTraffic = Delta < 90.f;

	const FVector CarLocal = SpotTransform.InverseTransformPosition(Car->GetActorLocation());
	const float RoomX = FMath::Max(1.f, Length * 0.5f - HalfL);
	const float Centred = 1.f - FMath::Clamp(FMath::Abs(CarLocal.X) / RoomX, 0.f, 1.f);
	const float Straight = 1.f - FMath::Clamp(AngleError / 12.f, 0.f, 1.f);
	Result.Neatness = 0.5f * Centred + 0.5f * Straight;
	return Result;
}
