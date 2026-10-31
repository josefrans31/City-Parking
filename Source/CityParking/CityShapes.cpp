#include "CityShapes.h"

#include "CityArtSettings.h"

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

UStaticMeshComponent* CityShapes::AddModel(AActor* Owner, USceneComponent* Parent, const FCityVehicleModel& Model,
	const FVector& GroundLocation, float Yaw, float TargetLengthCm)
{
	if (!Owner || Model.Mesh.IsNull())
	{
		return nullptr;
	}
	UStaticMesh* Mesh = Model.Mesh.LoadSynchronous();
	if (!Mesh)
	{
		return nullptr;
	}

	// Work out which way the model is long, and how much to scale it to the right length.
	const FBox Bounds = Mesh->GetBoundingBox();
	const FVector Size = Bounds.GetSize();
	const bool bLongAlongY = Size.Y > Size.X;
	const float Length = FMath::Max(Size.X, Size.Y);
	if (Length < 1.f)
	{
		return nullptr;
	}
	const float Scale = TargetLengthCm / Length;
	const FRotator Rot(0.f, Yaw + (bLongAlongY ? -90.f : 0.f) + (Model.bTurnAround ? 180.f : 0.f), 0.f);

	// Put the middle of the model's footprint, and the bottom of its tyres, on GroundLocation.
	const FVector Pivot(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Min.Z);
	const FVector Location = GroundLocation - Rot.RotateVector(Pivot * Scale) + FVector(0.f, 0.f, Model.RaiseBy);

	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
	C->SetStaticMesh(Mesh);
	C->SetMobility(EComponentMobility::Movable);
	C->SetupAttachment(Parent ? Parent : Owner->GetRootComponent());
	C->SetRelativeLocationAndRotation(Location, Rot);
	C->SetRelativeScale3D(FVector(Scale));
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetGenerateOverlapEvents(false);
	C->SetCanEverAffectNavigation(false);
	C->RegisterComponent();
	Owner->AddInstanceComponent(C);
	return C;
}

bool CityShapes::ApplySurface(UStaticMeshComponent* Component, const FCitySurface& Surface, const FVector& SizeCm)
{
	if (!Component || Surface.Material.IsNull())
	{
		return false;
	}
	UMaterialInterface* Base = Surface.Material.LoadSynchronous();
	if (!Base)
	{
		return false;
	}

	// The basic shapes stretch a texture once over each face, so ask the material to repeat it.
	// Use the two biggest sides, which is the face you look at most.
	TArray<float> Sides = { (float)SizeCm.X, (float)SizeCm.Y, (float)SizeCm.Z };
	Sides.Sort([](float A, float B) { return A > B; });
	const float Repeats = FMath::Max(1.f, (Sides[0] + Sides[1]) * 0.5f / FMath::Max(Surface.TileSizeCm, 10.f));

	UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Base, Component);
	static const TCHAR* CommonNames[] = {
		TEXT("Tiling"), TEXT("UV Tiling"), TEXT("UVTiling"), TEXT("Texture Tiling"),
		TEXT("Tiling X"), TEXT("Tiling Y"), TEXT("TilingX"), TEXT("TilingY"), TEXT("Tile"), TEXT("UV Scale")
	};
	const FName Custom = UCityArtSettings::Get().TilingParameterName;
	if (Custom != NAME_None)
	{
		Mat->SetScalarParameterValue(Custom, Repeats);
	}
	for (const TCHAR* Name : CommonNames)
	{
		Mat->SetScalarParameterValue(Name, Repeats); // does nothing if the material has no such setting
	}
	Component->SetMaterial(0, Mat);
	return true;
}
