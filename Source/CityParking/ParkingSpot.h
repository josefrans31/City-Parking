#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParkingSpot.generated.h"

class AParkingCar;
class UStaticMeshComponent;

// The open space you are hunting for: a green box on the street with a tall
// green beam you can see over the parked cars.
UCLASS()
class CITYPARKING_API AParkingSpot : public AActor
{
	GENERATED_BODY()

public:
	AParkingSpot();

	// Length runs along the street, Width across it (cm).
	void Init(float InLength, float InWidth);
	virtual void Tick(float DeltaSeconds) override;

	struct FCheck
	{
		bool bInside = false;        // all four corners of the car are in the box
		bool bStraight = false;      // car lines up with the curb
		bool bFacingTraffic = false; // pointing the way the one-way street goes
		float Neatness = 0.f;        // 0..1, how straight and centred
	};
	FCheck Check(const AParkingCar* Car) const;

	float Length = 700.f;
	float Width = 270.f;

private:
	UPROPERTY() TObjectPtr<USceneComponent> Root;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Beacon;
	float Time = 0.f;
};
