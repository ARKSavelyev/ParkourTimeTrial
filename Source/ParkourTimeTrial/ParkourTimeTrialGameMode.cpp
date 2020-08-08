// Copyright Epic Games, Inc. All Rights Reserved.

#include "ParkourTimeTrialGameMode.h"
#include "ParkourTimeTrialHUD.h"
#include "ParkourTimeTrialCharacter.h"
#include "UObject/ConstructorHelpers.h"

AParkourTimeTrialGameMode::AParkourTimeTrialGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AParkourTimeTrialHUD::StaticClass();
}
