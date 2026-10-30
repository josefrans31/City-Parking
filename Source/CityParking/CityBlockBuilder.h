#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CityBlockBuilder.generated.h"

class AParkingSpot;

// One one-way street. S runs along the street in the direction traffic goes,
// T runs across it (positive T is the right-hand side, toward the block).
struct FCityStreet
{
	FString Name;
	FVector2D Center = FVector2D::ZeroVector;
	FVector2D Dir = FVector2D(1, 0);
	float Length = 0.f;
	float Width = 0.f;

	FVector2D Right() const { return FVector2D(-Dir.Y, Dir.X); }
	float Yaw() const { return FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X)); }
	FVector ToWorld(float S, float T, float Z) const
	{
		const FVector2D P = Center + Dir * S + Right() * T;
		return FVector(P.X, P.Y, Z);
	}
	bool Contains(const FVector2D& P) const
	{
		const FVector2D D = P - Center;
		return FMath::Abs(FVector2D::DotProduct(D, Dir)) <= Length * 0.5f
			&& FMath::Abs(FVector2D::DotProduct(D, Right())) <= Width * 0.5f;
	}
};

// Builds one city block out of basic shapes when the level starts: four
// one-way streets in a loop, parked cars on both curbs, and the mess you
// have to squeeze past. The layout is random but the same for each level
// number, and gets harder as the level goes up.
UCLASS()
class CITYPARKING_API ACityBlockBuilder : public AActor
{
	GENERATED_BODY()

public:
	ACityBlockBuilder();

	void Build(int32 Level);

	// Filled in by Build, read by the game mode and the HUD.
	TArray<FCityStreet> Streets;
	FTransform PlayerStart;
	FVector2D MapHalfSize = FVector2D(7000, 4500);
	FVector2D BlockHalfSize = FVector2D(4350, 1850); // the sidewalk in the middle

	UPROPERTY() TObjectPtr<AParkingSpot> Spot;

	int32 FindStreet(const FVector& Location) const; // -1 at a corner or off the road

private:
	UPROPERTY() TObjectPtr<USceneComponent> Root;

	// A stretch of curb that must stay free of parked cars.
	struct FReserved { int32 Street; int32 Side; float S; float HalfLength; };
	TArray<FReserved> Reserved;
	bool IsReserved(int32 Street, int32 Side, float S, float HalfLength) const;

	void BuildGroundAndBuildings(FRandomStream& Rng);
	void BuildStreetPaint(const FCityStreet& Street);
	void AddCar(const FVector& Pos, float Yaw, const FLinearColor& Color, float Length, float Width, bool bHazards);
	void AddBus(int32 StreetIndex, float S, bool bMoving, FRandomStream& Rng);
	void AddPedestrian(const FVector& From, const FVector& To, FRandomStream& Rng);
	void AddGarbagePile(const FVector& Pos, FRandomStream& Rng);
	FLinearColor RandomCarColor(FRandomStream& Rng) const;
};
