#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MovingObstacle.generated.h"

class UBoxComponent;

// Something that moves back and forth between two points: a pedestrian on a
// crosswalk, a bus pulling out of its stop. It stops and waits if your car
// is in the way.
UCLASS()
class CITYPARKING_API AMovingObstacle : public AActor
{
	GENERATED_BODY()

public:
	AMovingObstacle();

	// Extent is half the size of the collision box. Tag says what it is ("Pedestrian", "Bus").
	void Setup(const FVector& InA, const FVector& InB, const FVector& Extent, float InSpeed,
		float InWaitA, float InWaitB, float StartDelay, bool bInFaceTravel, float FixedYaw, FName Tag);

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Obstacle")
	TObjectPtr<UBoxComponent> Box;

private:
	FVector A = FVector::ZeroVector;
	FVector B = FVector::ZeroVector;
	float Speed = 150.f;
	float WaitA = 2.f;
	float WaitB = 2.f;
	float WaitLeft = 0.f;
	bool bToB = true;
	bool bFaceTravel = false;

	void FaceTarget();
};
