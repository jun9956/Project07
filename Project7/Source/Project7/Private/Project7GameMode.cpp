// Copyright Epic Games, Inc. All Rights Reserved.
#include "Project7GameMode.h"

#include "Project7Character.h"
#include "Project7PlayerController.h"
#include "UObject/ConstructorHelpers.h"

AProject7GameMode::AProject7GameMode()
{
	// 채팅 컴포넌트가 장착된 PlayerController를 기본 클래스로 사용
	PlayerControllerClass = AProject7PlayerController::StaticClass();
	
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
