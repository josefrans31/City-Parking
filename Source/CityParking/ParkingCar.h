#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ParkingCar.generated.h"

class UBoxComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UStaticMeshComponent;

// The player's car. Arcade handling on a flat street: no wheel physics, so it
// steers exactly and squeezes through tight gaps. Bumping into things is
// reported to the game mode, which keeps the score.
UCLASS()
class CITYPARKING_API AParkingCar : public APawn
{
	GENERATED_BODY()

public:
	AParkingCar();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// Driving feel. All speeds in cm per second. Change these in a Blueprint child.
	UPROPERTY(EditAnywhere, Category = "Driving") float MaxForwardSpeed = 1500.f;   // about 34 mph
	UPROPERTY(EditAnywhere, Category = "Driving") float MaxReverseSpeed = 450.f;
	UPROPERTY(EditAnywhere, Category = "Driving") float Acceleration = 650.f;
	UPROPERTY(EditAnywhere, Category = "Driving") float BrakeDeceleration = 1800.f;
	UPROPERTY(EditAnywhere, Category = "Driving") float CoastDeceleration = 250.f;
	UPROPERTY(EditAnywhere, Category = "Driving") float HandbrakeDeceleration = 3200.f;
	UPROPERTY(EditAnywhere, Category = "Driving") float Wheelbase = 270.f;
	UPROPERTY(EditAnywhere, Category = "Driving") float MaxSteerAngle = 24.f;      // degrees. 24 gives about a 6 m turning circle radius, like a family car
	UPROPERTY(EditAnywhere, Category = "Driving") float SteerSpeed = 0.9f;         // how fast the wheel turns (full lock per second)
	// Tyre grip, in cm/s per second. At speed the steering is limited so the turn never needs more grip
	// than this, like a real car. Lower = wider turns at speed. 600 is about 0.6 g.
	UPROPERTY(EditAnywhere, Category = "Driving") float MaxSideGrip = 600.f;
	UPROPERTY(EditAnywhere, Category = "Driving") FLinearColor PaintColor = FLinearColor(0.75f, 0.05f, 0.04f);

	static constexpr float CarLength = 450.f;
	static constexpr float CarWidth = 190.f;
	static constexpr float RideHeight = 80.f; // centre of the collision box above the street

	float GetSpeed() const { return Speed; } // negative when reversing

	// The most the front wheels can turn right now. Full lock when slow, much less at speed.
	float CurrentSteerLimit() const;
	void SetControlEnabled(bool bEnabled) { bControlEnabled = bEnabled; }

	// The in-game settings menu (TAB). The HUD draws it; the car handles the keys.
	bool IsSettingsOpen() const { return bSettingsOpen; }
	int32 GetSettingsRow() const { return SettingsRow; }
	static constexpr int32 SettingsRows = 3; // steering angle, steering speed, reset

	UPROPERTY(VisibleAnywhere, Category = "Car") TObjectPtr<UBoxComponent> Body;
	UPROPERTY(VisibleAnywhere, Category = "Car") TObjectPtr<UStaticMeshComponent> BodyMesh;
	UPROPERTY(VisibleAnywhere, Category = "Car") TObjectPtr<UStaticMeshComponent> CabinMesh;
	UPROPERTY(VisibleAnywhere, Category = "Car") TArray<TObjectPtr<UStaticMeshComponent>> Wheels;
	UPROPERTY(VisibleAnywhere, Category = "Car") TObjectPtr<USpringArmComponent> CameraArm;
	UPROPERTY(VisibleAnywhere, Category = "Car") TObjectPtr<UCameraComponent> Camera;

private:
	float Speed = 0.f;
	float Steer = 0.f;
	float HitCooldown = 0.f;
	float FixedZ = RideHeight;
	bool bControlEnabled = true;
	bool bTopView = false;
	bool bSettingsOpen = false;
	int32 SettingsRow = 0;

	UPROPERTY() TObjectPtr<UInputMappingContext> Mapping;
	UPROPERTY() TObjectPtr<UInputAction> ThrottleAction;
	UPROPERTY() TObjectPtr<UInputAction> SteerAction;
	UPROPERTY() TObjectPtr<UInputAction> HandbrakeAction;
	UPROPERTY() TObjectPtr<UInputAction> CameraAction;
	UPROPERTY() TObjectPtr<UInputAction> RestartAction;
	UPROPERTY() TObjectPtr<UInputAction> NextAction;
	UPROPERTY() TObjectPtr<UInputAction> SettingsAction;
	UPROPERTY() TObjectPtr<UInputAction> MenuUpAction;
	UPROPERTY() TObjectPtr<UInputAction> MenuDownAction;
	UPROPERTY() TObjectPtr<UInputAction> MenuLeftAction;
	UPROPERTY() TObjectPtr<UInputAction> MenuRightAction;

	void BuildInput();
	float ReadAxis(const UInputAction* Action) const;
	void Drive(float DeltaSeconds);
	void ReportHit(const FHitResult& Hit, float ImpactSpeed);

	void ToggleCamera();
	void RequestRestart();
	void RequestNext();

	void ApplyPlayerSettings();
	void ToggleSettings();
	void MenuUp();
	void MenuDown();
	void MenuLeft() { MenuChange(-1); }
	void MenuRight() { MenuChange(1); }
	void MenuChange(int32 Direction);

	UFUNCTION()
	void OnBodyOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
