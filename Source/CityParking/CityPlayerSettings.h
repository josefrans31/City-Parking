#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CityPlayerSettings.generated.h"

// Settings the player changes in game (TAB opens the menu). They are saved to
// Saved/Config/<Platform>/GameUserSettings.ini, so they stick between plays.
UCLASS(Config = GameUserSettings)
class CITYPARKING_API UCityPlayerSettings : public UObject
{
	GENERATED_BODY()

public:
	// How far the front wheels turn, in degrees. Lower = wider, gentler turns.
	UPROPERTY(Config) float SteeringAngle = 24.f;

	// How quickly the wheel turns when you press left or right (full lock per second).
	UPROPERTY(Config) float SteeringSpeed = 0.9f;

	static constexpr float DefaultAngle = 24.f;
	static constexpr float MinAngle = 10.f;
	static constexpr float MaxAngle = 40.f;
	static constexpr float DefaultSpeed = 0.9f;
	static constexpr float MinSpeed = 0.3f;
	static constexpr float MaxSpeed = 3.0f;

	static UCityPlayerSettings* Get() { return GetMutableDefault<UCityPlayerSettings>(); }

	void ResetToDefaults()
	{
		SteeringAngle = DefaultAngle;
		SteeringSpeed = DefaultSpeed;
	}

	void Clamp()
	{
		SteeringAngle = FMath::Clamp(SteeringAngle, MinAngle, MaxAngle);
		SteeringSpeed = FMath::Clamp(SteeringSpeed, MinSpeed, MaxSpeed);
	}

	void Save()
	{
		Clamp();
		SaveConfig();
	}
};
