// Copyright Epic Games, Inc. All Rights Reserved.

#include "Project7GameMode.h"
#include "Project7Character.h"
#include "UObject/ConstructorHelpers.h"

AProject7GameMode::AProject7GameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
