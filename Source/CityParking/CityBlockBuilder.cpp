#include "CityBlockBuilder.h"

#include "CityShapes.h"
#include "MovingObstacle.h"
#include "ParkingCar.h"
#include "ParkingSpot.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"

namespace
{
	// Sizes in centimetres. The block is shorter than a real Manhattan block so a lap fits in a level.
	constexpr float BlockX = 4000.f;     // half length of the buildings in the middle
	constexpr float BlockY = 1500.f;     // half depth
	constexpr float Sidewalk = 350.f;
	constexpr float Road = 1300.f;       // curb to curb: parking lane, 8 m to drive in, parking lane
	constexpr float ParkLaneT = 525.f;   // centre of a parking lane, from the middle of the street
	constexpr float CurbT = Road * 0.5f; // 650
	constexpr float CarLen = 450.f;
	constexpr float CarWid = 190.f;

	const FLinearColor Asphalt(0.045f, 0.045f, 0.05f);
	const FLinearColor Concrete(0.45f, 0.44f, 0.42f);
	const FLinearColor Paint(0.85f, 0.85f, 0.8f);
	const FLinearColor Yellow(0.9f, 0.65f, 0.05f);
}

ACityBlockBuilder::ACityBlockBuilder()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
}

int32 ACityBlockBuilder::FindStreet(const FVector& Location) const
{
	const FVector2D P(Location.X, Location.Y);
	for (int32 i = 0; i < Streets.Num(); ++i)
	{
		if (Streets[i].Contains(P))
		{
			return i;
		}
	}
	return -1;
}

bool ACityBlockBuilder::IsReserved(int32 Street, int32 Side, float S, float HalfLength) const
{
	for (const FReserved& R : Reserved)
	{
		if (R.Street == Street && R.Side == Side && FMath::Abs(R.S - S) < R.HalfLength + HalfLength)
		{
			return true;
		}
	}
	return false;
}

FLinearColor ACityBlockBuilder::RandomCarColor(FRandomStream& Rng) const
{
	static const FLinearColor Colors[] = {
		FLinearColor(0.9f, 0.7f, 0.05f),   // yellow cab
		FLinearColor(0.02f, 0.02f, 0.02f), // black
		FLinearColor(0.8f, 0.8f, 0.8f),    // white
		FLinearColor(0.35f, 0.36f, 0.38f), // grey
		FLinearColor(0.05f, 0.12f, 0.35f), // navy
		FLinearColor(0.3f, 0.03f, 0.03f),  // dark red
		FLinearColor(0.1f, 0.25f, 0.12f),  // green
		FLinearColor(0.55f, 0.5f, 0.42f),  // beige
	};
	if (Rng.FRand() < 0.2f)
	{
		return Colors[0];
	}
	return Colors[Rng.RandRange(1, (int32)UE_ARRAY_COUNT(Colors) - 1)];
}

void ACityBlockBuilder::Build(int32 Level)
{
	const int32 Difficulty = FMath::Clamp(Level, 1, 3);
	FRandomStream Rng(Level * 7919 + 17);
	Reserved.Reset();

	const float InX = BlockX + Sidewalk, InY = BlockY + Sidewalk; // inner curb
	const float OutX = InX + Road, OutY = InY + Road;             // outer curb
	BlockHalfSize = FVector2D(InX, InY);
	MapHalfSize = FVector2D(OutX + Sidewalk + 1000.f, OutY + Sidewalk + 1000.f);

	// Four one-way streets. Going round the loop you always turn right.
	Streets.Reset();
	Streets.Add({ TEXT("11th Street"), FVector2D(0, -(InY + OutY) * 0.5f), FVector2D(1, 0), 2 * InX, Road });
	Streets.Add({ TEXT("Avenue B"), FVector2D((InX + OutX) * 0.5f, 0), FVector2D(0, 1), 2 * InY, Road });
	Streets.Add({ TEXT("12th Street"), FVector2D(0, (InY + OutY) * 0.5f), FVector2D(-1, 0), 2 * InX, Road });
	Streets.Add({ TEXT("Avenue A"), FVector2D(-(InX + OutX) * 0.5f, 0), FVector2D(0, -1), 2 * InY, Road });

	BuildGroundAndBuildings(Rng);
	for (const FCityStreet& Street : Streets)
	{
		BuildStreetPaint(Street);
	}

	// Player starts at the west end of 11th Street, heading east.
	const FCityStreet& First = Streets[0];
	const float StartS = -First.Length * 0.5f + 800.f;
	PlayerStart = FTransform(FRotator(0, First.Yaw(), 0), First.ToWorld(StartS, 0, AParkingCar::RideHeight));
	Reserved.Add({ 0, 1, StartS + 600.f, 1500.f });
	Reserved.Add({ 0, -1, StartS + 600.f, 1500.f });

	// --- The parking spot ---
	const int32 SpotStreet = Level == 1 ? 1 : Level == 2 ? 2 : Level == 3 ? 3 : Rng.RandRange(1, 3);
	const int32 SpotSide = Level == 2 ? -1 : (Level <= 3 ? 1 : (Rng.FRand() < 0.5f ? 1 : -1));
	const float SpotLen = Level == 1 ? 1100.f : Level == 2 ? 850.f : Level == 3 ? 720.f : 650.f;
	const float SpotWid = 270.f;
	const FCityStreet& SpotSt = Streets[SpotStreet];
	const float SpotRange = SpotSt.Length * 0.5f - 1300.f;
	const float SpotS = Rng.FRandRange(-SpotRange, SpotRange);
	{
		const float SpotT = SpotSide * (CurbT - SpotWid * 0.5f);
		Spot = GetWorld()->SpawnActor<AParkingSpot>(AParkingSpot::StaticClass(),
			FTransform(FRotator(0, SpotSt.Yaw(), 0), SpotSt.ToWorld(SpotS, SpotT, 0.f)));
		if (Spot)
		{
			Spot->Init(SpotLen, SpotWid);
		}
		Reserved.Add({ SpotStreet, SpotSide, SpotS, SpotLen * 0.5f });
	}

	// --- Buses at their stops on the long streets ---
	for (int32 i : { 0, 2 })
	{
		// On 11th Street the bus stays between the start and the construction zone.
		float BusS = i == 0 ? Rng.FRandRange(-1200.f, 0.f) : Rng.FRandRange(-1500.f, 1500.f);
		if (i == SpotStreet && SpotSide == -1 && FMath::Abs(BusS - SpotS) < SpotLen * 0.5f + 900.f)
		{
			BusS = SpotS + (SpotS > 0.f ? -1.f : 1.f) * (SpotLen * 0.5f + 1000.f);
		}
		const bool bMoving = Difficulty >= 2;
		Reserved.Add({ i, -1, BusS, 800.f });
		AddBus(i, BusS, bMoving, Rng);
	}

	// --- Garbage truck with two workers (level 2+), on Avenue A ---
	if (Difficulty >= 2 && !(SpotStreet == 3 && SpotSide == 1 && Level <= 3))
	{
		const FCityStreet& St = Streets[3];
		const float TruckS = Rng.FRandRange(-600.f, 600.f);
		const float TruckT = 400.f - 125.f;
		Reserved.Add({ 3, 1, TruckS, 650.f });
		const FVector P = St.ToWorld(TruckS, TruckT, 0);
		const FRotator R(0, St.Yaw(), 0);
		CityShapes::Add(this, Root, CityShapes::Cube(), P + FVector(0, 0, 160), FVector(800, 250, 300), R, FLinearColor(0.15f, 0.4f, 0.15f), EShapeCollision::SolidNoCamera, TEXT("Truck"));
		CityShapes::Add(this, Root, CityShapes::Cube(), P + R.RotateVector(FVector(300, 0, 330)), FVector(200, 240, 80), R, FLinearColor(0.85f, 0.85f, 0.85f), EShapeCollision::None);
		for (int32 w = 0; w < 2; ++w)
		{
			const float WS = TruckS - 450.f - w * 60.f;
			AddPedestrian(St.ToWorld(WS, CurbT + 150.f, 110.f), St.ToWorld(WS, 150.f, 110.f), Rng);
		}
	}

	// --- Construction zone narrowing 11th Street (level 3+) ---
	if (Difficulty >= 3)
	{
		const FCityStreet& St = Streets[0];
		const float ZoneS = 1800.f;
		Reserved.Add({ 0, 1, ZoneS, 900.f });
		const FRotator R(0, St.Yaw(), 0);
		CityShapes::Add(this, Root, CityShapes::Cube(), St.ToWorld(ZoneS, 420.f, 60.f), FVector(1600, 460, 120), R, FLinearColor(0.9f, 0.35f, 0.02f), EShapeCollision::SolidNoCamera, TEXT("Barrier"));
		for (int32 c = -3; c <= 3; ++c)
		{
			CityShapes::Add(this, Root, CityShapes::Cone(), St.ToWorld(ZoneS + c * 260.f, 150.f, 40.f), FVector(45, 45, 80), FRotator::ZeroRotator, FLinearColor(1.f, 0.3f, 0.f), EShapeCollision::SolidNoCamera, TEXT("Cone"));
		}
	}

	// --- Double-parked cars and delivery vans sticking into the lane ---
	for (int32 i = 0; i < Streets.Num(); ++i)
	{
		const FCityStreet& St = Streets[i];
		const bool bShort = St.Length < 5000.f;
		const int32 Count = bShort ? FMath::Max(1, Difficulty - 1) : Difficulty + 1;
		int32 Placed = 0;
		for (int32 Try = 0; Try < 30 && Placed < Count; ++Try)
		{
			const int32 Side = Rng.FRand() < 0.5f ? 1 : -1;
			const float S = Rng.FRandRange(-St.Length * 0.5f + 900.f, St.Length * 0.5f - 900.f);
			const bool bVan = Rng.FRand() < 0.4f;
			const float Len = bVan ? 600.f : CarLen;
			const float Wid = bVan ? 220.f : CarWid;
			// Keep the lane open in front of the spot until level 3.
			const bool bNearSpot = i == SpotStreet && FMath::Abs(S - SpotS) < SpotLen * 0.5f + 800.f;
			if ((bNearSpot && Difficulty < 3) || IsReserved(i, Side, S, Len * 0.5f + 400.f) || IsReserved(i, -Side, S, Len * 0.5f + 200.f))
			{
				continue;
			}
			const float T = Side * (400.f + 10.f - Wid * 0.5f);
			Reserved.Add({ i, Side, S, Len * 0.5f + 100.f });
			const FLinearColor Color = bVan ? (Rng.FRand() < 0.5f ? FLinearColor(0.85f, 0.85f, 0.85f) : FLinearColor(0.35f, 0.22f, 0.08f)) : RandomCarColor(Rng);
			AddCar(St.ToWorld(S, T, 0), St.Yaw() + Rng.FRandRange(-6.f, 6.f), Color, Len, Wid, true);
			++Placed;
		}
	}

	// --- Parked cars along both curbs ---
	for (int32 i = 0; i < Streets.Num(); ++i)
	{
		const FCityStreet& St = Streets[i];
		const float SMin = -St.Length * 0.5f + 450.f + CarLen * 0.5f;
		const float SMax = St.Length * 0.5f - 450.f - CarLen * 0.5f;
		for (int32 Side : { 1, -1 })
		{
			TArray<float> Slots;
			if (i == SpotStreet && Side == SpotSide)
			{
				// Cars packed right up to the open spot, so you have to parallel park.
				for (float S = SpotS - SpotLen * 0.5f - CarLen * 0.5f - 15.f; S >= SMin; S -= CarLen + Rng.FRandRange(60.f, 140.f)) Slots.Add(S);
				for (float S = SpotS + SpotLen * 0.5f + CarLen * 0.5f + 15.f; S <= SMax; S += CarLen + Rng.FRandRange(60.f, 140.f)) Slots.Add(S);
			}
			else
			{
				for (float S = SMin + Rng.FRandRange(0.f, 120.f); S <= SMax; S += CarLen + Rng.FRandRange(70.f, 160.f))
				{
					if (Rng.FRand() > 0.1f) Slots.Add(S); // the odd gap is a hydrant or a driveway
				}
			}
			for (float S : Slots)
			{
				if (IsReserved(i, Side, S, CarLen * 0.5f))
				{
					continue;
				}
				const float T = Side * (ParkLaneT + Rng.FRandRange(-12.f, 12.f));
				AddCar(St.ToWorld(S, T, 0), St.Yaw() + Rng.FRandRange(-2.f, 2.f), RandomCarColor(Rng), CarLen, CarWid, false);
			}
		}
	}

	// --- Garbage bags piled at the curb, spilling into the lane ---
	for (int32 i = 0; i < Streets.Num(); ++i)
	{
		const FCityStreet& St = Streets[i];
		const int32 Piles = (St.Length < 5000.f ? 1 : 2) + Difficulty;
		for (int32 p = 0; p < Piles; ++p)
		{
			const int32 Side = Rng.FRand() < 0.5f ? 1 : -1;
			const float S = Rng.FRandRange(-St.Length * 0.5f + 700.f, St.Length * 0.5f - 700.f);
			if ((i == 0 && S < -St.Length * 0.5f + 1500.f) || IsReserved(i, Side, S, 200.f))
			{
				continue; // not right in front of the start, the spot or a bus stop
			}
			AddGarbagePile(St.ToWorld(S, Side * 370.f, 40.f), Rng);
		}
		// More bags up on the sidewalk, just for looks.
		for (int32 p = 0; p < 3; ++p)
		{
			const int32 Side = Rng.FRand() < 0.5f ? 1 : -1;
			AddGarbagePile(St.ToWorld(Rng.FRandRange(-St.Length * 0.4f, St.Length * 0.4f), Side * (CurbT + 80.f), 55.f), Rng);
		}
	}

	// --- People on the crosswalks ---
	const int32 Walkers = Difficulty * 2;
	for (int32 n = 0; n < Walkers; ++n)
	{
		const int32 i = n % Streets.Num();
		const FCityStreet& St = Streets[i];
		const float End = (n / Streets.Num()) % 2 == 0 ? 1.f : -1.f;
		const float S = End * (St.Length * 0.5f - 170.f) + Rng.FRandRange(-60.f, 60.f);
		const bool bFlip = Rng.FRand() < 0.5f;
		const FVector A = St.ToWorld(S, (bFlip ? 1 : -1) * (CurbT + 150.f), 110.f);
		const FVector B = St.ToWorld(S, (bFlip ? -1 : 1) * (CurbT + 150.f), 110.f);
		AddPedestrian(A, B, Rng);
	}
}

void ACityBlockBuilder::BuildGroundAndBuildings(FRandomStream& Rng)
{
	const float InX = BlockX + Sidewalk, InY = BlockY + Sidewalk;
	const float OutX = InX + Road, OutY = InY + Road;
	const float FarX = OutX + Sidewalk, FarY = OutY + Sidewalk;
	const FRotator Flat = FRotator::ZeroRotator;

	// Street surface.
	CityShapes::Add(this, Root, CityShapes::Cube(), FVector(0, 0, -50), FVector(40000, 32000, 100), Flat, Asphalt, EShapeCollision::Solid, TEXT("Ground"));

	// Sidewalks are 15 cm tall, so the curb stops the car.
	CityShapes::Add(this, Root, CityShapes::Cube(), FVector(0, 0, 7.5f), FVector(2 * InX, 2 * InY, 15), Flat, Concrete, EShapeCollision::Solid, TEXT("Curb"));
	CityShapes::Add(this, Root, CityShapes::Cube(), FVector(0, -(OutY + FarY) * 0.5f, 7.5f), FVector(2 * FarX, Sidewalk, 15), Flat, Concrete, EShapeCollision::Solid, TEXT("Curb"));
	CityShapes::Add(this, Root, CityShapes::Cube(), FVector(0,  (OutY + FarY) * 0.5f, 7.5f), FVector(2 * FarX, Sidewalk, 15), Flat, Concrete, EShapeCollision::Solid, TEXT("Curb"));
	CityShapes::Add(this, Root, CityShapes::Cube(), FVector(-(OutX + FarX) * 0.5f, 0, 7.5f), FVector(Sidewalk, 2 * FarY, 15), Flat, Concrete, EShapeCollision::Solid, TEXT("Curb"));
	CityShapes::Add(this, Root, CityShapes::Cube(), FVector( (OutX + FarX) * 0.5f, 0, 7.5f), FVector(Sidewalk, 2 * FarY, 15), Flat, Concrete, EShapeCollision::Solid, TEXT("Curb"));

	static const FLinearColor Facades[] = {
		FLinearColor(0.42f, 0.17f, 0.11f), // brick
		FLinearColor(0.30f, 0.19f, 0.13f), // brownstone
		FLinearColor(0.55f, 0.53f, 0.50f), // limestone
		FLinearColor(0.68f, 0.60f, 0.48f), // beige
		FLinearColor(0.25f, 0.32f, 0.40f), // glass
		FLinearColor(0.33f, 0.33f, 0.35f), // grey
	};
	auto AddBuilding = [&](const FVector2D& Center, const FVector2D& Size)
	{
		const float Height = Rng.FRandRange(1600.f, 6500.f);
		const FLinearColor Color = Facades[Rng.RandRange(0, (int32)UE_ARRAY_COUNT(Facades) - 1)];
		CityShapes::Add(this, Root, CityShapes::Cube(), FVector(Center.X, Center.Y, 15 + Height * 0.5f), FVector(Size.X, Size.Y, Height), Flat, Color, EShapeCollision::Solid, TEXT("Building"));
		// Darker shopfront band at street level.
		CityShapes::Add(this, Root, CityShapes::Cube(), FVector(Center.X, Center.Y, 15 + 200), FVector(Size.X + 20, Size.Y + 20, 400), Flat, Color * 0.35f, EShapeCollision::None);
	};

	// The block in the middle: a row of buildings.
	const int32 Inner = 6;
	const float W = 2 * BlockX / Inner;
	for (int32 b = 0; b < Inner; ++b)
	{
		AddBuilding(FVector2D(-BlockX + W * (b + 0.5f), 0), FVector2D(W - 30, 2 * BlockY));
	}

	// The blocks all around the outside.
	const float Depth = 3000.f;
	const int32 Row = 8;
	const float RowW = (2 * FarX + 2 * Depth) / Row;
	for (int32 b = 0; b < Row; ++b)
	{
		const float X = -FarX - Depth + RowW * (b + 0.5f);
		AddBuilding(FVector2D(X, -FarY - Depth * 0.5f), FVector2D(RowW - 30, Depth));
		AddBuilding(FVector2D(X,  FarY + Depth * 0.5f), FVector2D(RowW - 30, Depth));
	}
	for (int32 b = 0; b < 3; ++b)
	{
		const float H = 2 * FarY / 3;
		const float Y = -FarY + H * (b + 0.5f);
		AddBuilding(FVector2D(-FarX - Depth * 0.5f, Y), FVector2D(Depth, H - 30));
		AddBuilding(FVector2D( FarX + Depth * 0.5f, Y), FVector2D(Depth, H - 30));
	}
}

void ACityBlockBuilder::BuildStreetPaint(const FCityStreet& St)
{
	const FRotator R(0, St.Yaw(), 0);

	// One-way arrows down the middle.
	for (float S = -St.Length * 0.5f + 1200.f; S < St.Length * 0.5f - 600.f; S += 2200.f)
	{
		CityShapes::Add(this, Root, CityShapes::Cube(), St.ToWorld(S, 0, 0.5f), FVector(260, 28, 1), R, Paint, EShapeCollision::None);
		for (float Side : { 1.f, -1.f })
		{
			const FRotator Head(0, St.Yaw() + Side * 145.f, 0);
			CityShapes::Add(this, Root, CityShapes::Cube(), St.ToWorld(S + 100.f, Side * 45.f, 0.5f), FVector(130, 26, 1), Head, Paint, EShapeCollision::None);
		}
	}

	// Crosswalks at both ends.
	for (float End : { 1.f, -1.f })
	{
		const float S = End * (St.Length * 0.5f - 170.f);
		for (float T = -CurbT + 50.f; T < CurbT; T += 110.f)
		{
			CityShapes::Add(this, Root, CityShapes::Cube(), St.ToWorld(S, T, 0.5f), FVector(280, 55, 1), R, Paint, EShapeCollision::None);
		}
	}

	// One Way sign on the right-hand corner where the street begins.
	const FVector Post = St.ToWorld(-St.Length * 0.5f + 40.f, CurbT + 120.f, 15.f);
	CityShapes::Add(this, Root, CityShapes::Cylinder(), Post + FVector(0, 0, 150), FVector(10, 10, 300), FRotator::ZeroRotator, FLinearColor(0.3f, 0.3f, 0.3f), EShapeCollision::Solid, TEXT("Curb"));
	CityShapes::Add(this, Root, CityShapes::Cube(), Post + FVector(0, 0, 280), FVector(90, 4, 30), R, FLinearColor(0.01f, 0.01f, 0.01f), EShapeCollision::None);
	CityShapes::Add(this, Root, CityShapes::Cube(), Post + FVector(0, 0, 280) + R.RotateVector(FVector(0, -3, 0)), FVector(70, 4, 8), R, Paint, EShapeCollision::None);

	// Street lamps and the odd fire hydrant along both sidewalks.
	for (float Side : { 1.f, -1.f })
	{
		for (float S = -St.Length * 0.5f + 900.f; S < St.Length * 0.5f - 600.f; S += 2400.f)
		{
			const FVector Lamp = St.ToWorld(S, Side * (CurbT + 60.f), 15.f);
			CityShapes::Add(this, Root, CityShapes::Cylinder(), Lamp + FVector(0, 0, 400), FVector(16, 16, 800), FRotator::ZeroRotator, FLinearColor(0.12f, 0.14f, 0.12f), EShapeCollision::Solid, TEXT("Curb"));
			CityShapes::Add(this, Root, CityShapes::Cube(), Lamp + FVector(0, 0, 790) + FVector(St.Right() * -Side * 60.f, 0), FVector(70, 30, 15), FRotator::ZeroRotator, FLinearColor(1.f, 0.9f, 0.6f), EShapeCollision::None);
		}
		const FVector Hydrant = St.ToWorld(Side * St.Length * 0.2f, Side * (CurbT + 45.f), 15.f);
		CityShapes::Add(this, Root, CityShapes::Cylinder(), Hydrant + FVector(0, 0, 40), FVector(30, 30, 80), FRotator::ZeroRotator, FLinearColor(0.7f, 0.05f, 0.03f), EShapeCollision::Solid, TEXT("Curb"));
	}
}

void ACityBlockBuilder::AddCar(const FVector& Pos, float Yaw, const FLinearColor& Color, float Length, float Width, bool bHazards)
{
	const FRotator R(0, Yaw, 0);
	auto At = [&](const FVector& Local) { return Pos + R.RotateVector(Local); };
	const bool bVan = Length > 500.f;

	CityShapes::Add(this, Root, CityShapes::Cube(), At(FVector(0, 0, 65)), FVector(Length, Width, 70), R, Color, EShapeCollision::SolidNoCamera, TEXT("Car"));
	if (bVan)
	{
		CityShapes::Add(this, Root, CityShapes::Cube(), At(FVector(-60, 0, 170)), FVector(Length - 140, Width, 150), R, Color, EShapeCollision::None);
	}
	else
	{
		CityShapes::Add(this, Root, CityShapes::Cube(), At(FVector(-30, 0, 125)), FVector(240, Width - 20, 55), R, Color * 0.6f, EShapeCollision::None);
	}
	if (Color.R > 0.8f && Color.G > 0.55f && Color.B < 0.1f)
	{
		// Taxi light on the roof.
		CityShapes::Add(this, Root, CityShapes::Cube(), At(FVector(-30, 0, 162)), FVector(60, 30, 18), R, FLinearColor(1.f, 1.f, 0.85f), EShapeCollision::None);
	}
	for (const FVector& Wheel : { FVector(Length * 0.32f, Width * 0.46f, 35), FVector(Length * 0.32f, -Width * 0.46f, 35), FVector(-Length * 0.32f, Width * 0.46f, 35), FVector(-Length * 0.32f, -Width * 0.46f, 35) })
	{
		CityShapes::Add(this, Root, CityShapes::Cylinder(), At(Wheel), FVector(70, 70, 24), FRotator(0, Yaw, 90), FLinearColor(0.02f, 0.02f, 0.02f), EShapeCollision::None);
	}
	if (bHazards)
	{
		// Blinkers on: "I'll just be a minute."
		for (float Side : { 1.f, -1.f })
		{
			CityShapes::Add(this, Root, CityShapes::Cube(), At(FVector(Length * 0.5f, Side * (Width * 0.5f - 15), 80)), FVector(6, 22, 12), R, FLinearColor(1.f, 0.45f, 0.f), EShapeCollision::None);
			CityShapes::Add(this, Root, CityShapes::Cube(), At(FVector(-Length * 0.5f, Side * (Width * 0.5f - 15), 80)), FVector(6, 22, 12), R, FLinearColor(1.f, 0.45f, 0.f), EShapeCollision::None);
		}
	}
}

void ACityBlockBuilder::AddBus(int32 StreetIndex, float S, bool bMoving, FRandomStream& Rng)
{
	const FCityStreet& St = Streets[StreetIndex];
	const float Yaw = St.Yaw();
	const FVector AtStop = St.ToWorld(S, -(CurbT - 140.f), 165.f);
	const FVector PulledOut = St.ToWorld(S, -170.f, 165.f);

	AMovingObstacle* Bus = GetWorld()->SpawnActor<AMovingObstacle>(AMovingObstacle::StaticClass(), FTransform(AtStop));
	if (!Bus)
	{
		return;
	}
	// A bus that is pulling out waits at the stop, eases into the lane, sits there, then goes back.
	Bus->Setup(AtStop, bMoving ? PulledOut : AtStop, FVector(600, 130, 140), 120.f,
		Rng.FRandRange(4.f, 7.f), Rng.FRandRange(4.f, 6.f), Rng.FRandRange(1.f, 5.f), false, Yaw, TEXT("Bus"));

	const FRotator Flat = FRotator::ZeroRotator;
	CityShapes::Add(Bus, Bus->Box, CityShapes::Cube(), FVector(0, 0, 0), FVector(1200, 260, 280), Flat, FLinearColor(0.85f, 0.86f, 0.88f), EShapeCollision::None);
	CityShapes::Add(Bus, Bus->Box, CityShapes::Cube(), FVector(0, 0, 40), FVector(1160, 264, 70), Flat, FLinearColor(0.03f, 0.05f, 0.08f), EShapeCollision::None);
	CityShapes::Add(Bus, Bus->Box, CityShapes::Cube(), FVector(0, 0, -60), FVector(1204, 264, 25), Flat, FLinearColor(0.05f, 0.25f, 0.7f), EShapeCollision::None);
	CityShapes::Add(Bus, Bus->Box, CityShapes::Cube(), FVector(598, 0, 110), FVector(8, 160, 30), Flat, FLinearColor(1.f, 0.6f, 0.f), EShapeCollision::None);

	// Bus stop sign and shelter on the sidewalk.
	const FVector Stop = St.ToWorld(S + 400.f, -(CurbT + 90.f), 15.f);
	CityShapes::Add(this, Root, CityShapes::Cylinder(), Stop + FVector(0, 0, 150), FVector(8, 8, 300), Flat, FLinearColor(0.3f, 0.3f, 0.3f), EShapeCollision::Solid, TEXT("Curb"));
	CityShapes::Add(this, Root, CityShapes::Cube(), Stop + FVector(0, 0, 270), FVector(45, 45, 45), Flat, FLinearColor(0.05f, 0.25f, 0.7f), EShapeCollision::None);
	CityShapes::Add(this, Root, CityShapes::Cube(), St.ToWorld(S - 200.f, -(CurbT + 230.f), 140.f), FVector(400, 120, 250), FRotator(0, Yaw, 0), FLinearColor(0.2f, 0.25f, 0.3f), EShapeCollision::Solid, TEXT("Curb"));
}

void ACityBlockBuilder::AddPedestrian(const FVector& From, const FVector& To, FRandomStream& Rng)
{
	AMovingObstacle* Person = GetWorld()->SpawnActor<AMovingObstacle>(AMovingObstacle::StaticClass(), FTransform(From));
	if (!Person)
	{
		return;
	}
	Person->Setup(From, To, FVector(30, 30, 90), Rng.FRandRange(110.f, 170.f),
		Rng.FRandRange(2.f, 6.f), Rng.FRandRange(2.f, 6.f), Rng.FRandRange(0.f, 6.f), true, 0.f, TEXT("Pedestrian"));

	static const FLinearColor Clothes[] = {
		FLinearColor(0.6f, 0.1f, 0.1f), FLinearColor(0.1f, 0.2f, 0.6f), FLinearColor(0.1f, 0.1f, 0.1f),
		FLinearColor(0.7f, 0.6f, 0.2f), FLinearColor(0.2f, 0.5f, 0.3f), FLinearColor(0.8f, 0.8f, 0.8f),
	};
	const FLinearColor Shirt = Clothes[Rng.RandRange(0, (int32)UE_ARRAY_COUNT(Clothes) - 1)];
	const FLinearColor Skin = FLinearColor::LerpUsingHSV(FLinearColor(0.95f, 0.75f, 0.6f), FLinearColor(0.25f, 0.13f, 0.07f), Rng.FRand());
	CityShapes::Add(Person, Person->Box, CityShapes::Cylinder(), FVector(0, 0, -55), FVector(36, 36, 70), FRotator::ZeroRotator, FLinearColor(0.08f, 0.08f, 0.15f), EShapeCollision::None);
	CityShapes::Add(Person, Person->Box, CityShapes::Cylinder(), FVector(0, 0, 10), FVector(48, 48, 65), FRotator::ZeroRotator, Shirt, EShapeCollision::None);
	CityShapes::Add(Person, Person->Box, CityShapes::Sphere(), FVector(0, 0, 62), FVector(30, 30, 32), FRotator::ZeroRotator, Skin, EShapeCollision::None);
}

void ACityBlockBuilder::AddGarbagePile(const FVector& Pos, FRandomStream& Rng)
{
	const int32 Bags = Rng.RandRange(3, 6);
	for (int32 b = 0; b < Bags; ++b)
	{
		const FVector Offset(Rng.FRandRange(-90.f, 90.f), Rng.FRandRange(-50.f, 50.f), b * 15.f);
		const float Size = Rng.FRandRange(55.f, 80.f);
		const FLinearColor Color = Rng.FRand() < 0.7f ? FLinearColor(0.02f, 0.02f, 0.02f) : FLinearColor(0.1f, 0.18f, 0.08f);
		CityShapes::Add(this, Root, CityShapes::Sphere(), Pos + Offset, FVector(Size, Size, Size * 0.85f), FRotator(0, Rng.FRandRange(0.f, 360.f), 0), Color, EShapeCollision::Physics, TEXT("Garbage"));
	}
}
