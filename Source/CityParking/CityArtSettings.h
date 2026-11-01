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

// Which way to spin a building model so its front faces the street.
UENUM(BlueprintType)
enum class ECityTurn : uint8
{
	None    UMETA(DisplayName = "No turn"),
	Quarter UMETA(DisplayName = "90 degrees"),
	Half    UMETA(DisplayName = "180 degrees"),
	ThreeQuarter UMETA(DisplayName = "270 degrees"),
};

// One building model you downloaded (for example a building from a Fab city pack).
USTRUCT(BlueprintType)
struct FCityBuildingModel
{
	GENERATED_BODY()

	// The building. Leave empty to keep the box building.
	UPROPERTY(EditAnywhere, Category = "Model") TSoftObjectPtr<UStaticMesh> Mesh;

	// Spin the building if its front door faces away from the street.
	UPROPERTY(EditAnywhere, Category = "Model") ECityTurn Turn = ECityTurn::None;

	// Off: the building keeps its shape and fits inside its spot (there may be gaps).
	// On: it is stretched to fill the whole spot, which can squash the windows a bit.
	UPROPERTY(EditAnywhere, Category = "Model") bool bFillTheSpot = false;

	// Nudge the building up or down if it floats or sinks into the sidewalk. In cm.
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

	// --- Buildings ---
	// Real buildings. Add a few and each spot along the street picks one. You still bump into
	// the same walls as before, so the streets and parking do not change.
	UPROPERTY(Config, EditAnywhere, Category = "Buildings") TArray<FCityBuildingModel> Buildings;

	// --- Textures ---
	UPROPERTY(Config, EditAnywhere, Category = "Textures") FCitySurface Street;
	UPROPERTY(Config, EditAnywhere, Category = "Textures") FCitySurface Sidewalk;

	// Building walls for the box buildings (not used where a real building model is placed). Add a few (brick, stone, concrete) and each building picks one.
	UPROPERTY(Config, EditAnywhere, Category = "Textures") TArray<FCitySurface> BuildingWalls;

	// The name of the "how many repeats" setting inside your materials. Open one of your
	// material instances and look for it if textures look stretched. Common names are tried too.
	UPROPERTY(Config, EditAnywhere, Category = "Textures") FName TilingParameterName = TEXT("Tiling");

	static const UCityArtSettings& Get() { return *GetDefault<UCityArtSettings>(); }
};
