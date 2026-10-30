#include "MovingObstacle.h"

#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"

AMovingObstacle::AMovingObstacle()
{
	PrimaryActorTick.bCanEverTick = true;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	Box->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore); // walk through garbage bags
	Box->SetMobility(EComponentMobility::Movable);
	RootComponent = Box;
}

void AMovingObstacle::Setup(const FVector& InA, const FVector& InB, const FVector& Extent, float InSpeed,
	float InWaitA, float InWaitB, float StartDelay, bool bInFaceTravel, float FixedYaw, FName Tag)
{
	A = InA;
	B = InB;
	Speed = InSpeed;
	WaitA = InWaitA;
	WaitB = InWaitB;
	WaitLeft = StartDelay;
	bFaceTravel = bInFaceTravel;
	bToB = true;

	Box->SetBoxExtent(Extent);
	if (Tag != NAME_None)
	{
		Box->ComponentTags.Add(Tag);
	}
	SetActorLocation(A);
	SetActorRotation(FRotator(0.f, FixedYaw, 0.f));
	if (bFaceTravel)
	{
		FaceTarget();
	}
}

void AMovingObstacle::FaceTarget()
{
	const FVector To = (bToB ? B : A) - GetActorLocation();
	if (!To.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.f, To.Rotation().Yaw, 0.f));
	}
}

void AMovingObstacle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (WaitLeft > 0.f)
	{
		WaitLeft -= DeltaSeconds;
		return;
	}

	const FVector Target = bToB ? B : A;
	FVector ToTarget = Target - GetActorLocation();
	ToTarget.Z = 0.f;
	const float Dist = ToTarget.Size();

	if (Dist < 2.f)
	{
		WaitLeft = bToB ? WaitB : WaitA;
		bToB = !bToB;
		if (bFaceTravel)
		{
			FaceTarget();
		}
		return;
	}

	// Sweep, so we stop instead of driving through the player's car.
	FHitResult Hit;
	AddActorWorldOffset(ToTarget / Dist * FMath::Min(Speed * DeltaSeconds, Dist), true, &Hit);
}
