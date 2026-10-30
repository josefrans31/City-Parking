#include "ParkingGameMode.h"

#include "CityBlockBuilder.h"
#include "ParkingCar.h"
#include "ParkingHUD.h"
#include "ParkingSpot.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FLinearColor Red(1.f, 0.25f, 0.2f);
	const FLinearColor Orange(1.f, 0.6f, 0.1f);
	const FLinearColor Green(0.3f, 1.f, 0.4f);
	const FLinearColor White(1.f, 1.f, 1.f);
}

AParkingGameMode::AParkingGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AParkingCar::StaticClass();
	HUDClass = AParkingHUD::StaticClass();
}

void AParkingGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	Level = FMath::Max(1, UGameplayStatics::GetIntOption(Options, TEXT("Level"), 1));
	TimeLimit = Level == 1 ? 150.f : Level == 2 ? 120.f : Level == 3 ? 100.f : FMath::Max(70.f, 100.f - 5.f * (Level - 3));
	TimeLeft = TimeLimit;
}

void AParkingGameMode::EnsureLighting()
{
	UWorld* World = GetWorld();
	// A level made from the "Basic" template already has these. An empty level gets them here.
	if (!TActorIterator<ADirectionalLight>(World))
	{
		const FTransform Sun(FRotator(-40.f, 35.f, 0.f));
		ADirectionalLight* Light = World->SpawnActorDeferred<ADirectionalLight>(ADirectionalLight::StaticClass(), Sun);
		if (Light)
		{
			Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);
			if (UDirectionalLightComponent* SunLight = Cast<UDirectionalLightComponent>(Light->GetLightComponent()))
			{
				SunLight->bAtmosphereSunLight = true;
			}
			Light->FinishSpawning(Sun);
		}
	}
	if (!TActorIterator<ASkyAtmosphere>(World))
	{
		World->SpawnActor<ASkyAtmosphere>(ASkyAtmosphere::StaticClass(), FTransform::Identity);
	}
	if (!TActorIterator<ASkyLight>(World))
	{
		ASkyLight* Sky = World->SpawnActorDeferred<ASkyLight>(ASkyLight::StaticClass(), FTransform::Identity);
		if (Sky)
		{
			Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
			Sky->GetLightComponent()->bRealTimeCapture = true;
			Sky->FinishSpawning(FTransform::Identity);
		}
	}
}

void AParkingGameMode::EnsureBuilt()
{
	if (City)
	{
		return;
	}
	EnsureLighting();
	City = GetWorld()->SpawnActor<ACityBlockBuilder>(ACityBlockBuilder::StaticClass(), FTransform::Identity);
	if (City)
	{
		City->Build(Level);
	}
}

APawn* AParkingGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	// The block decides where the car starts, so any level works, PlayerStart or not.
	EnsureBuilt();
	if (City)
	{
		return SpawnDefaultPawnAtTransform(NewPlayer, City->PlayerStart);
	}
	return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
}

void AParkingGameMode::BeginPlay()
{
	Super::BeginPlay();
	EnsureBuilt();
	AddMessage(FString::Printf(TEXT("LEVEL %d: find the green P and park in it"), Level), White, 4.f);
	AddMessage(TEXT("One-way streets: follow the arrows"), Orange, 4.f);
}

AParkingSpot* AParkingGameMode::GetSpot() const
{
	return City ? City->Spot.Get() : nullptr;
}

AParkingCar* AParkingGameMode::GetCar() const
{
	return Cast<AParkingCar>(UGameplayStatics::GetPlayerPawn(this, 0));
}

void AParkingGameMode::AddMessage(const FString& Text, const FLinearColor& Color, float Seconds)
{
	Messages.Add({ Text, Color, Seconds });
	if (Messages.Num() > 4)
	{
		Messages.RemoveAt(0);
	}
}

void AParkingGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (int32 i = Messages.Num() - 1; i >= 0; --i)
	{
		Messages[i].TimeLeft -= DeltaSeconds;
		if (Messages[i].TimeLeft <= 0.f)
		{
			Messages.RemoveAt(i);
		}
	}

	AParkingCar* Car = GetCar();
	if (State != EParkingState::Driving || !Car || !City)
	{
		return;
	}

	TimeLeft -= DeltaSeconds;
	if (TimeLeft <= 0.f)
	{
		TimeLeft = 0.f;
		Finish(false, TEXT("Out of time. You got a parking ticket!"));
		return;
	}

	// Which street are we on, and are we going the wrong way down it?
	const int32 StreetIndex = City->FindStreet(Car->GetActorLocation());
	bWrongWay = false;
	if (StreetIndex >= 0)
	{
		const FCityStreet& Street = City->Streets[StreetIndex];
		StreetName = Street.Name;
		const FVector Forward = Car->GetActorForwardVector();
		const float Facing = FVector2D::DotProduct(FVector2D(Forward.X, Forward.Y), Street.Dir);
		// Backing up into a spot is fine. Driving forward against the arrows is not.
		bWrongWay = Facing < -0.5f && Car->GetSpeed() > 150.f;
	}
	WrongWayTime = bWrongWay ? WrongWayTime + DeltaSeconds : 0.f;
	if (WrongWayTime > 1.f)
	{
		WrongWayPenalty += 25.f * DeltaSeconds;
		if (WrongWayPenalty >= 5.f)
		{
			Score -= 5;
			WrongWayPenalty -= 5.f;
		}
	}

	// Parked? All four corners in the box, straight, and stopped for a moment.
	if (AParkingSpot* Spot = GetSpot())
	{
		const AParkingSpot::FCheck Check = Spot->Check(Car);
		bInSpot = Check.bInside;
		if (Check.bInside && Check.bStraight && FMath::Abs(Car->GetSpeed()) < 25.f)
		{
			ParkHold += DeltaSeconds;
			if (ParkHold >= ParkHoldNeeded)
			{
				const int32 TimeBonus = FMath::RoundToInt(TimeLeft * 10.f);
				const int32 CarBonus = FMath::RoundToInt((100.f - Damage) * 5.f);
				const int32 NeatBonus = FMath::RoundToInt(Check.Neatness * 200.f);
				const int32 TrafficBonus = Check.bFacingTraffic ? 150 : 0;
				Breakdown.Reset();
				Breakdown.Add(FString::Printf(TEXT("Driving score       %d"), Score));
				Breakdown.Add(FString::Printf(TEXT("Time left bonus     +%d"), TimeBonus));
				Breakdown.Add(FString::Printf(TEXT("No-scratch bonus    +%d"), CarBonus));
				Breakdown.Add(FString::Printf(TEXT("Neat parking        +%d"), NeatBonus));
				Breakdown.Add(Check.bFacingTraffic ? TEXT("Facing traffic      +150") : TEXT("Parked backwards    +0"));
				FinalScore = FMath::Max(0, Score + TimeBonus + CarBonus + NeatBonus + TrafficBonus);
				Stars = (Damage < 10.f && TimeLeft > TimeLimit * 0.3f) ? 3 : (Damage < 40.f ? 2 : 1);
				Finish(true, TEXT("Nice parking!"));
			}
		}
		else
		{
			ParkHold = 0.f;
		}
	}
}

void AParkingGameMode::ReportCollision(UPrimitiveComponent* What, float ImpactSpeed)
{
	if (State != EParkingState::Driving || !What)
	{
		return;
	}

	if (What->ComponentHasTag(TEXT("Pedestrian")))
	{
		Damage += 3.f;
		Score -= 250;
		AddMessage(TEXT("Watch out for pedestrians!  -250"), Red);
	}
	else
	{
		float Hurt = FMath::Clamp(ImpactSpeed / 45.f, 1.f, 30.f);
		FString What2 = TEXT("Scraped a car");
		if (What->ComponentHasTag(TEXT("Curb")) || What->ComponentHasTag(TEXT("Building")))
		{
			Hurt *= 0.4f;
			What2 = TEXT("Hit the curb");
		}
		else if (What->ComponentHasTag(TEXT("Bus")))
		{
			What2 = TEXT("Bumped the bus");
		}
		else if (What->ComponentHasTag(TEXT("Truck")))
		{
			What2 = TEXT("Hit the garbage truck");
		}
		else if (What->ComponentHasTag(TEXT("Barrier")) || What->ComponentHasTag(TEXT("Cone")))
		{
			What2 = TEXT("Hit the road works");
		}
		const int32 Points = FMath::RoundToInt(Hurt * 8.f);
		Damage += Hurt;
		Score -= Points;
		AddMessage(FString::Printf(TEXT("%s  -%d"), *What2, Points), Orange);
	}

	if (Damage >= 100.f)
	{
		Damage = 100.f;
		Finish(false, TEXT("Your car is totaled!"));
	}
}

void AParkingGameMode::ReportGarbage()
{
	if (State != EParkingState::Driving)
	{
		return;
	}
	Score -= 20;
	AddMessage(TEXT("Ran over garbage  -20"), Orange, 1.5f);
}

void AParkingGameMode::Finish(bool bParked, const FString& Reason)
{
	State = bParked ? EParkingState::Parked : EParkingState::Failed;
	EndTitle = bParked ? TEXT("PARKED!") : TEXT("NO PARKING TODAY");
	EndReason = Reason;
	if (!bParked)
	{
		FinalScore = 0;
		Stars = 0;
		Breakdown.Reset();
	}
	if (AParkingCar* Car = GetCar())
	{
		Car->SetControlEnabled(false);
	}
}

void AParkingGameMode::RestartLevel()
{
	const FString Map = UGameplayStatics::GetCurrentLevelName(this, true);
	UGameplayStatics::OpenLevel(this, FName(*Map), true, FString::Printf(TEXT("Level=%d"), Level));
}

void AParkingGameMode::NextLevel()
{
	if (State != EParkingState::Parked)
	{
		return;
	}
	const FString Map = UGameplayStatics::GetCurrentLevelName(this, true);
	UGameplayStatics::OpenLevel(this, FName(*Map), true, FString::Printf(TEXT("Level=%d"), Level + 1));
}
