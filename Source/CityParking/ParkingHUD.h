#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ParkingHUD.generated.h"

class AParkingGameMode;

// Draws everything on screen with plain shapes and text, no UI assets needed:
// clock, score, damage, speed, an arrow to the spot, a mini map and the
// end-of-level card. Move to UMG widgets once the look is decided.
UCLASS()
class CITYPARKING_API AParkingHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	float UI = 1.f; // scale: 1.0 at 1080p

	void Text(const FString& S, float X, float Y, const FLinearColor& Color, float Scale, bool bCentre = false);
	void Bar(float X, float Y, float W, float H, float Fill, const FLinearColor& Color);
	void DrawArrowToSpot(AParkingGameMode* Game);
	void DrawMiniMap(AParkingGameMode* Game);
	void DrawEndCard(AParkingGameMode* Game);
};
