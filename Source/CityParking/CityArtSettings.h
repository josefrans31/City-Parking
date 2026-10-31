#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CityArtSettings.generated.h"

class UMaterialInterface;
class UStaticMesh;

// One vehicle model you downloaded (for example from Fab).
USTRUCT(BlueprintType)
struct FCityVehicleModel
{
	GENERATED_BODY()

	// The model itself. Leave empty to keep the coloured boxes.
	UPROPERTY(EditAnywhere, Category = "Model") TSoftObjectPtr<UStaticMesh> Mesh;

	// Turn this on if the model drives backwards (its nose points the wrong way).
	UPROPERTY(EditAnywhere, Category = "Model") bool bTurnAround = false;

	// Nudge the model up or down if the tyres float or sink into the street. In cm.
	UPROPERTY(EditAnywhere, Category = "Model") float RaiseBy = 0.f;
};

// A texture (material) for a surface, for example a Megascans asphalt.
USTRUCT(BlueprintType)
struct FCitySurface
{
	GENERATED_BODY()

	// The material. Leave empty to keep the flat colour.
	UPROPERTY(EditAnywhere, Category = "Surface") TSoftObjectPtr<UMaterialInterface> Material;

	// How big one copy of the texture is, in cm. Smaller = more repeats.
	UPROPERTY(EditAnywhere, Category = "Surface", meta = (ClampMin = "10")) float TileSizeCm = 400.f;
};

// Real models and textures for the city. Find this in the editor under
// Edit > Project Settings > Game > City Parking Art. Anything left empty
// keeps the simple shapes, so the game always runs.
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "City Parking Art"))
class CITYPARKING_API UCityArtSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UCityArtSettings();

	// --- Vehicles ---
	UPROPERTY(Config, EditAnywhere, Category = "Vehicles") FCityVehicleModel PlayerCar;

	// Parked and double-parked cars. Add a few and the street picks at random.
	UPROPERTY(Config, EditAnywhere, Category = "Vehicles") TArray<FCityVehicleModel> ParkedCars;

	// Delivery vans that double-park.
	UPROPERTY(Config, EditAnywhere, Category = "Vehicles") TArray<FCityVehicleModel> Vans;

	UPROPERTY(Config, EditAnywhere, Category = "Vehicles") FCityVehicleModel Bus;

	// --- Textures ---
	UPROPERTY(Config, EditAnywhere, Category = "Textures") FCitySurface Street;
	UPROPERTY(Config, EditAnywhere, Category = "Textures") FCitySurface Sidewalk;

	// Building walls. Add a few (brick, stone, concrete) and each building picks one.
	UPROPERTY(Config, EditAnywhere, Category = "Textures") TArray<FCitySurface> BuildingWalls;

	// The name of the "how many repeats" setting inside your materials. Open one of your
	// material instances and look for it if textures look stretched. Common names are tried too.
	UPROPERTY(Config, EditAnywhere, Category = "Textures") FName TilingParameterName = TEXT("Tiling");

	static const UCityArtSettings& Get() { return *GetDefault<UCityArtSettings>(); }
};
