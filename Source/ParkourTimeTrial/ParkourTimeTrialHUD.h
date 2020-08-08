// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ParkourTimeTrialHUD.generated.h"

UCLASS()
class AParkourTimeTrialHUD : public AHUD
{
	GENERATED_BODY()

public:
	AParkourTimeTrialHUD();

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

};

