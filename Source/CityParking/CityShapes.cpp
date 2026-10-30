#include "CityShapes.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

UStaticMesh* CityShapes::Cube()     { return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")); }
UStaticMesh* CityShapes::Cylinder() { return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")); }
UStaticMesh* CityShapes::Sphere()   { return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")); }
UStaticMesh* CityShapes::Cone()     { return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone")); }

void CityShapes::Paint(UStaticMeshComponent* Component, const FLinearColor& Color)
{
	static const TCHAR* BasePath = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, BasePath);
	if (!Component || !Base)
	{
		return;
	}
	UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Base, Component);
	Mat->SetVectorParameterValue(TEXT("Color"), Color);
	Component->SetMaterial(0, Mat);
}

UStaticMeshComponent* CityShapes::Add(AActor* Owner, USceneComponent* Parent, UStaticMesh* Mesh,
	const FVector& Location, const FVector& SizeCm, const FRotator& Rotation,
	const FLinearColor& Color, EShapeCollision Collision, FName Tag)
{
	if (!Owner || !Mesh)
	{
		return nullptr;
	}

	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
	C->SetStaticMesh(Mesh);
	C->SetMobility(EComponentMobility::Movable);
	C->SetupAttachment(Parent ? Parent : Owner->GetRootComponent());
	C->SetRelativeLocationAndRotation(Location, Rotation);
	C->SetRelativeScale3D(SizeCm / 100.f); // basic shapes are 100 cm
	C->SetCanEverAffectNavigation(false);

	switch (Collision)
	{
	case EShapeCollision::None:
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetGenerateOverlapEvents(false);
		break;
	case EShapeCollision::Solid:
		C->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		C->SetGenerateOverlapEvents(false);
		break;
	case EShapeCollision::SolidNoCamera:
		C->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		C->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		C->SetGenerateOverlapEvents(false);
		break;
	case EShapeCollision::Physics:
		C->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
		C->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		C->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		C->SetGenerateOverlapEvents(true);
		break;
	}

	if (Tag != NAME_None)
	{
		C->ComponentTags.Add(Tag);
	}

	C->RegisterComponent();
	Owner->AddInstanceComponent(C);
	Paint(C, Color);

	if (Collision == EShapeCollision::Physics)
	{
		C->SetSimulatePhysics(true);
		C->SetMassOverrideInKg(NAME_None, 6.f, true);
		C->SetLinearDamping(0.6f);
		C->SetAngularDamping(0.6f);
	}
	return C;
}
