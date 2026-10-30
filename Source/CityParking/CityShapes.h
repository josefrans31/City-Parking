#pragma once

#include "CoreMinimal.h"

class AActor;
class UStaticMesh;
class UStaticMeshComponent;
class USceneComponent;

// How a shape collides with the player's car.
enum class EShapeCollision : uint8
{
	None,           // paint, decorations
	Solid,          // buildings, curbs: block the car and the camera
	SolidNoCamera,  // cars, buses: block the car, the camera looks through them
	Physics         // garbage bags: the car knocks them around
};

// Everything in the game is built from the engine's basic shapes, so the
// project needs no art files to run. Swap in real models later.
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
}
