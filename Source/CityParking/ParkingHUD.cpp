#include "ParkingHUD.h"

#include "CityBlockBuilder.h"
#include "ParkingCar.h"
#include "ParkingGameMode.h"
#include "ParkingSpot.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"

namespace
{
	const FLinearColor Panel(0.f, 0.f, 0.f, 0.5f);
	const FLinearColor White(1.f, 1.f, 1.f);
	const FLinearColor Grey(0.7f, 0.7f, 0.7f);
	const FLinearColor Red(1.f, 0.25f, 0.2f);
	const FLinearColor Green(0.3f, 1.f, 0.4f);
	const FLinearColor Yellow(1.f, 0.85f, 0.2f);
}

void AParkingHUD::Text(const FString& S, float X, float Y, const FLinearColor& Color, float Scale, bool bCentre)
{
	UFont* Font = GEngine->GetLargeFont();
	if (bCentre)
	{
		float W = 0.f, H = 0.f;
		GetTextSize(S, W, H, Font, Scale * UI);
		X -= W * 0.5f;
	}
	DrawText(S, FLinearColor(0, 0, 0, 0.8f), X + 2.f, Y + 2.f, Font, Scale * UI); // shadow
	DrawText(S, Color, X, Y, Font, Scale * UI);
}

void AParkingHUD::Bar(float X, float Y, float W, float H, float Fill, const FLinearColor& Color)
{
	DrawRect(FLinearColor(0.1f, 0.1f, 0.1f, 0.8f), X, Y, W, H);
	DrawRect(Color, X, Y, W * FMath::Clamp(Fill, 0.f, 1.f), H);
}

void AParkingHUD::DrawHUD()
{
	Super::DrawHUD();
	AParkingGameMode* Game = GetWorld() ? GetWorld()->GetAuthGameMode<AParkingGameMode>() : nullptr;
	if (!Game || !Canvas)
	{
		return;
	}
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	UI = H / 1080.f;

	// Top left: level, clock, score, damage.
	DrawRect(Panel, 20 * UI, 20 * UI, 380 * UI, 175 * UI);
	Text(FString::Printf(TEXT("LEVEL %d"), Game->Level), 36 * UI, 28 * UI, Yellow, 1.4f);
	const int32 Secs = FMath::CeilToInt(Game->TimeLeft);
	Text(FString::Printf(TEXT("TIME  %d:%02d"), Secs / 60, Secs % 60), 36 * UI, 70 * UI, Secs <= 15 ? Red : White, 1.4f);
	Text(FString::Printf(TEXT("SCORE  %d"), Game->Score), 36 * UI, 110 * UI, White, 1.2f);
	Text(TEXT("DAMAGE"), 36 * UI, 152 * UI, Grey, 0.9f);
	Bar(140 * UI, 156 * UI, 240 * UI, 20 * UI, Game->Damage / 100.f, Game->Damage > 60.f ? Red : Yellow);

	// Top centre: street name and arrow to the spot.
	if (!Game->StreetName.IsEmpty())
	{
		Text(Game->StreetName + TEXT("   ONE WAY"), W * 0.5f, 24 * UI, White, 1.1f, true);
	}
	DrawArrowToSpot(Game);

	// Bottom left: speed.
	if (AParkingCar* Car = Game->GetCar())
	{
		const int32 Mph = FMath::RoundToInt(FMath::Abs(Car->GetSpeed()) * 0.0223694f);
		DrawRect(Panel, 20 * UI, H - 110 * UI, 220 * UI, 90 * UI);
		Text(FString::Printf(TEXT("%d MPH"), Mph), 36 * UI, H - 100 * UI, White, 2.0f);
		Text(Car->GetSpeed() < -5.f ? TEXT("REVERSE") : TEXT("DRIVE"), 36 * UI, H - 48 * UI, Grey, 0.9f);
	}

	// Middle: messages and warnings.
	float MsgY = H * 0.28f;
	if (Game->bWrongWay && FMath::Fmod(GetWorld()->GetTimeSeconds(), 0.6f) < 0.4f)
	{
		Text(TEXT("WRONG WAY!  ONE WAY STREET"), W * 0.5f, MsgY, Red, 2.2f, true);
	}
	MsgY += 60 * UI;
	for (const AParkingGameMode::FMessage& M : Game->Messages)
	{
		Text(M.Text, W * 0.5f, MsgY, M.Color, 1.2f, true);
		MsgY += 36 * UI;
	}

	// Parking: hold still in the box.
	if (Game->State == EParkingState::Driving && Game->bInSpot)
	{
		const float Fill = Game->ParkHold / AParkingGameMode::ParkHoldNeeded;
		Text(Fill > 0.f ? TEXT("HOLD IT...") : TEXT("STRAIGHTEN UP AND STOP"), W * 0.5f, H * 0.7f, Green, 1.4f, true);
		Bar(W * 0.5f - 150 * UI, H * 0.7f + 45 * UI, 300 * UI, 18 * UI, Fill, Green);
	}

	DrawMiniMap(Game);

	// Bottom centre: controls.
	if (Game->State == EParkingState::Driving)
	{
		Text(TEXT("W/S drive & reverse   A/D steer   SPACE brake   C camera   R restart"), W * 0.5f, H - 40 * UI, Grey, 0.8f, true);
	}
	else
	{
		DrawEndCard(Game);
	}
}

void AParkingHUD::DrawArrowToSpot(AParkingGameMode* Game)
{
	AParkingCar* Car = Game->GetCar();
	AParkingSpot* Spot = Game->GetSpot();
	if (!Car || !Spot)
	{
		return;
	}
	const FVector ToSpot = Spot->GetActorLocation() - Car->GetActorLocation();
	const FVector Local = Car->GetActorTransform().InverseTransformVectorNoScale(ToSpot);
	const float Angle = FMath::Atan2(Local.Y, Local.X); // 0 = straight ahead, + = to the right

	const float CX = Canvas->ClipX * 0.5f;
	const float CY = 105 * UI;
	const float Len = 32 * UI;
	const FVector2D Dir(FMath::Sin(Angle), -FMath::Cos(Angle)); // screen up is ahead
	const FVector2D Tip = FVector2D(CX, CY) + Dir * Len;
	const FVector2D Tail = FVector2D(CX, CY) - Dir * Len;
	const FVector2D Side(-Dir.Y, Dir.X);
	DrawLine(Tail.X, Tail.Y, Tip.X, Tip.Y, Green, 5 * UI);
	const FVector2D L = Tip - Dir * 18 * UI + Side * 14 * UI;
	const FVector2D R = Tip - Dir * 18 * UI - Side * 14 * UI;
	DrawLine(Tip.X, Tip.Y, L.X, L.Y, Green, 5 * UI);
	DrawLine(Tip.X, Tip.Y, R.X, R.Y, Green, 5 * UI);
	Text(FString::Printf(TEXT("P  %d m"), FMath::RoundToInt(ToSpot.Size2D() / 100.f)), CX, CY + 40 * UI, Green, 1.0f, true);
}

void AParkingHUD::DrawMiniMap(AParkingGameMode* Game)
{
	ACityBlockBuilder* City = Game->GetCity();
	if (!City)
	{
		return;
	}
	const FVector2D Half = City->MapHalfSize;
	const float MW = 320 * UI;
	const float MH = MW * Half.Y / Half.X;
	const float X0 = Canvas->ClipX - MW - 20 * UI;
	const float Y0 = Canvas->ClipY - MH - 20 * UI;
	auto ToMap = [&](const FVector2D& P) { return FVector2D(X0 + (P.X + Half.X) / (2 * Half.X) * MW, Y0 + (P.Y + Half.Y) / (2 * Half.Y) * MH); };

	DrawRect(FLinearColor(0.15f, 0.12f, 0.1f, 0.85f), X0, Y0, MW, MH);
	for (const FCityStreet& St : City->Streets)
	{
		// Half size of the street in world X/Y, stretched over the corners.
		const float AlongHalf = (St.Length + St.Width) * 0.5f;
		const FVector2D HalfSize = St.Dir.GetAbs() * AlongHalf + St.Right().GetAbs() * St.Width * 0.5f;
		const FVector2D A = ToMap(St.Center - HalfSize);
		const FVector2D B = ToMap(St.Center + HalfSize);
		DrawRect(FLinearColor(0.35f, 0.35f, 0.38f, 0.95f), FMath::Min(A.X, B.X), FMath::Min(A.Y, B.Y), FMath::Abs(B.X - A.X), FMath::Abs(B.Y - A.Y));
		// Little arrow showing which way the street goes.
		const FVector2D M = ToMap(St.Center);
		const FVector2D Tip = M + St.Dir * 14 * UI;
		DrawLine(M.X - St.Dir.X * 14 * UI, M.Y - St.Dir.Y * 14 * UI, Tip.X, Tip.Y, White, 2 * UI);
		DrawLine(Tip.X, Tip.Y, Tip.X - St.Dir.X * 6 * UI + St.Dir.Y * 6 * UI, Tip.Y - St.Dir.Y * 6 * UI - St.Dir.X * 6 * UI, White, 2 * UI);
		DrawLine(Tip.X, Tip.Y, Tip.X - St.Dir.X * 6 * UI - St.Dir.Y * 6 * UI, Tip.Y - St.Dir.Y * 6 * UI + St.Dir.X * 6 * UI, White, 2 * UI);
	}
	if (AParkingSpot* Spot = Game->GetSpot())
	{
		const FVector2D P = ToMap(FVector2D(Spot->GetActorLocation()));
		DrawRect(Green, P.X - 5 * UI, P.Y - 5 * UI, 10 * UI, 10 * UI);
	}
	if (AParkingCar* Car = Game->GetCar())
	{
		const FVector2D P = ToMap(FVector2D(Car->GetActorLocation()));
		const FVector F = Car->GetActorForwardVector();
		DrawRect(Red, P.X - 4 * UI, P.Y - 4 * UI, 8 * UI, 8 * UI);
		DrawLine(P.X, P.Y, P.X + F.X * 14 * UI, P.Y + F.Y * 14 * UI, Red, 2 * UI);
	}
}

void AParkingHUD::DrawEndCard(AParkingGameMode* Game)
{
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	DrawRect(FLinearColor(0, 0, 0, 0.55f), 0, 0, W, H);

	const bool bParked = Game->State == EParkingState::Parked;
	float Y = H * 0.25f;
	Text(Game->EndTitle, W * 0.5f, Y, bParked ? Green : Red, 3.0f, true);
	Y += 90 * UI;
	Text(Game->EndReason, W * 0.5f, Y, White, 1.3f, true);
	Y += 60 * UI;

	if (bParked)
	{
		for (const FString& Line : Game->Breakdown)
		{
			Text(Line, W * 0.5f, Y, Grey, 1.1f, true);
			Y += 34 * UI;
		}
		Y += 10 * UI;
		Text(FString::Printf(TEXT("TOTAL  %d"), Game->FinalScore), W * 0.5f, Y, Yellow, 2.0f, true);
		Y += 70 * UI;

		// Stars as little boxes, filled or empty.
		const float Size = 36 * UI;
		const float StartX = W * 0.5f - Size * 2.5f;
		for (int32 i = 0; i < 3; ++i)
		{
			const float X = StartX + i * Size * 2.f;
			DrawRect(i < Game->Stars ? Yellow : FLinearColor(0.25f, 0.25f, 0.25f), X, Y, Size, Size);
		}
		Y += Size + 40 * UI;
		Text(TEXT("N  next level        R  play again"), W * 0.5f, Y, White, 1.2f, true);
	}
	else
	{
		Text(TEXT("R  try again"), W * 0.5f, Y + 20 * UI, White, 1.3f, true);
	}
}
