#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Project7PlayerController.generated.h"

// 플러그인의 채팅 컴포넌트를 전방 선언
class UMultiplayerChatComponent;

UCLASS()
class PROJECT7_API AProject7PlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AProject7PlayerController();

private:
	// 이 플레이어의 채팅 송수신을 담당하는 컴포넌트
	// Blueprint와 UMG에서도 읽을 수 있도록 공개
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Multiplayer Chat",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<UMultiplayerChatComponent> MultiplayerChatComponent;
};
