#include "Project7PlayerController.h"

#include "MultiplayerChatComponent.h"

AProject7PlayerController::AProject7PlayerController()
{
	// PlayerController가 생성될 때 채팅 컴포넌트를 함께 생성
	MultiplayerChatComponent = CreateDefaultSubobject<UMultiplayerChatComponent>(TEXT("MultiplayerChatComponent"));
}