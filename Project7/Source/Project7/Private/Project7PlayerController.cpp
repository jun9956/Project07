#include "Project7PlayerController.h"

#include "MultiplayerChatComponent.h"

AProject7PlayerController::AProject7PlayerController()
{
	// PlayerController가 생성될 때 채팅 컴포넌트를 함께 생성
	MultiplayerChatComponent = CreateDefaultSubobject<UMultiplayerChatComponent>(TEXT("MultiplayerChatComponent"));
}

void AProject7PlayerController::ChatTest(const FString& MessageText)
{
	// 채팅 컴포넌트가 없으면 명령을 실행하지 않습니다.
	if (MultiplayerChatComponent == nullptr)
	{
		return;
	}

	// 입력한 메시지를 전체 채팅으로 전송
	MultiplayerChatComponent->SendGlobalMessage(MessageText);
}