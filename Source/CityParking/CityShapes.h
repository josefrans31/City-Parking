#pragma once

#include "CoreMinimal.h"

class AActor;
class UStaticMesh;
class UStaticMeshComponent;
class USceneComponent;
struct FCitySurface;
struct FCityVehicleModel;
struct FCityBuildingModel;

// How a shape collides with the player's car.
enum class EShapeCollision : uint8
{
	None,           // paint, decorations
	Solid,          // buildings, curbs: block the car and the camera
	SolidNoCamera,  // cars, buses: block the car, the camera looks through them
	Physics         // garbage bags: the car knocks them around
};

// Everything in the game is built from the engine's basic shapes, so the
// project needs no art files to run. Real models and textures picked in
// Project Settings > City Parking Art replace them (see CityArtSettings).
namespace CityShapes
{
	UStaticMesh* Cube();
	UStaticMesh* Cylinder();
	UStaticMesh* Sphere();
	UStaticMesh* Cone();

	// Adds a shape to Owner. SizeCm is the full size in centimetres.
	// Location and Rotation are relative to Parent.
	UStaticMeshComponent* Add(AActor* Owner, USceneComponent* Parent, UStaticMesh* Mesh,
		const FVector& Location, const FVector& SizeCm, const FRotator& Rotation,
		const FLinearColor& Color, EShapeCollision Collision, FName Tag = NAME_None);

	// Gives a mesh component a flat colour.
	void Paint(UStaticMeshComponent* Component, const FLinearColor& Color);

	// Adds a downloaded vehicle model, scaled to TargetLengthCm long, with its tyres on
	// GroundLocation and its nose pointing along Yaw. No collision: keep a hidden box for that.
	// Returns nullptr if no model is set, so the caller can fall back to boxes.
	UStaticMeshComponent* AddModel(AActor* Owner, USceneComponent* Parent, const FCityVehicleModel& Model,
		const FVector& GroundLocation, float Yaw, float TargetLengthCm);

	// Adds a downloaded building, sized to fit a spot of SpotSizeCm (X by Y), standing on
	// GroundLocation with its front (the model's +X side) facing FrontYaw. No collision.
	// Returns nullptr if no model is set, so the caller can keep the box building.
	UStaticMeshComponent* AddBuilding(AActor* Owner, USceneComponent* Parent, const FCityBuildingModel& Model,
		const FVector& GroundLocation, const FVector2D& SpotSizeCm, float FrontYaw);

	// Puts a texture on a shape instead of its flat colour, repeating it so it is not stretched.
	// SizeCm is the shape's full size. Returns false (and changes nothing) if no texture is set.
	bool ApplySurface(UStaticMeshComponent* Component, const FCitySurface& Surface, const FVector& SizeCm);
}
