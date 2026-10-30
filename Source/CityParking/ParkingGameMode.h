#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ParkingGameMode.generated.h"

class ACityBlockBuilder;
class AParkingCar;
class AParkingSpot;

UENUM()
enum class EParkingState : uint8
{
	Driving,
	Parked,
	Failed
};

// Runs one level: builds the block, counts down the clock, keeps score and
// decides when you have parked (or failed). Levels are numbered from 1 and
// passed between restarts as "?Level=N".
UCLASS()
class CITYPARKING_API AParkingGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AParkingGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Called by the car.
	void ReportCollision(UPrimitiveComponent* What, float ImpactSpeed);
	void ReportGarbage();
	void RestartLevel();
	void NextLevel();

	// Read by the HUD.
	int32 Level = 1;
	EParkingState State = EParkingState::Driving;
	float TimeLimit = 150.f;
	float TimeLeft = 150.f;
	float Damage = 0.f;          // 0..100, the car is totaled at 100
	int32 Score = 1000;
	int32 FinalScore = 0;
	int32 Stars = 0;
	float ParkHold = 0.f;        // seconds held still inside the spot
	static constexpr float ParkHoldNeeded = 1.5f;
	bool bWrongWay = false;
	bool bInSpot = false;
	FString StreetName;
	FString EndTitle;
	FString EndReason;
	TArray<FString> Breakdown;

	struct FMessage { FString Text; FLinearColor Color; float TimeLeft; };
	TArray<FMessage> Messages;

	ACityBlockBuilder* GetCity() const { return City; }
	AParkingSpot* GetSpot() const;
	AParkingCar* GetCar() const;

private:
	UPROPERTY() TObjectPtr<ACityBlockBuilder> City;
	float WrongWayTime = 0.f;
	float WrongWayPenalty = 0.f;

	void EnsureBuilt();
	void EnsureLighting();
	void Finish(bool bParked, const FString& Reason);
	void AddMessage(const FString& Text, const FLinearColor& Color, float Seconds = 2.5f);
};
