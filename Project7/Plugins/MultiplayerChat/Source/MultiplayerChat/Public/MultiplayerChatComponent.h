#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MultiplayerChatTypes.h"
#include "MultiplayerChatComponent.generated.h"

// 클라이언트가 채팅 메시지를 수신했을 때 발생하는 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnMultiplayerChatMessageReceived,
	const FMultiplayerChatMessage&,
	Message
);

// 플레이어의 채팅 송수신을 담당하는 네트워크 컴포넌트
// 이 컴포넌트는 PlayerController에 장착할 예정
// 클라이언트가 보낸 메시지를 서버에 전달하고,
// 서버가 승인한 메시지를 다시 클라이언트에 전달
UCLASS(ClassGroup = (MultiplayerChat), meta = (BlueprintSpawnableComponent))
class MULTIPLAYERCHAT_API UMultiplayerChatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMultiplayerChatComponent();

	// 전체 채팅 메시지를 서버로 전송
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Chat")
	void SendGlobalMessage(const FString& MessageText);
	
	// 서버에서 승인된 메시지를 수신할 때 호출되는 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Multiplayer Chat")
	FOnMultiplayerChatMessageReceived OnMessageReceived;

private:
	
	// 클라이언트가 서버로 메시지를 보낼 때 사용하는 RPC
	UFUNCTION(Server, Reliable)
	void ServerSendGlobalMessage(const FString& MessageText);

	// 서버가 특정 클라이언트에 메시지를 전달할 때 사용하는 RPC
	UFUNCTION(Client, Reliable)
	void ClientReceiveMessage(const FMultiplayerChatMessage& Message);

	// 서버에서 도배 방지를 계산하기 위해 사용하는 마지막 전송 시각
	double LastAcceptedMessageTime = -1.0;
};
