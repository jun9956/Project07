#include "MultiplayerChatComponent.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

UMultiplayerChatComponent::UMultiplayerChatComponent()
{
	
	PrimaryComponentTick.bCanEverTick = false;

	// Server RPC와 Client RPC를 사용할 수 있도록 컴포넌트 복제를 활성화
	SetIsReplicatedByDefault(true);
}

void UMultiplayerChatComponent::SendGlobalMessage(const FString& MessageText)
{
	// 사용자 입력의 앞뒤 공백을 제거
	FString TrimmedMessage = MessageText;
	TrimmedMessage.TrimStartAndEndInline();

	// 비어 있는 메시지는 서버로 전송하지 않습니다.
	if (TrimmedMessage.IsEmpty())
	{
		return;
	}

	// 소유 클라이언트에서 서버 RPC를 호출
	ServerSendGlobalMessage(TrimmedMessage);
}

void UMultiplayerChatComponent::ServerSendGlobalMessage_Implementation(
	const FString& MessageText)
{
	constexpr int32 MaximumMessageLength = 256;
	constexpr double MinimumMessageInterval = 0.5;

	UWorld* World = GetWorld();
	APlayerController* SenderController = Cast<APlayerController>(GetOwner());

	// 유효한 월드와 PlayerController가 없으면 메시지를 처리하지 않습니다.
	if (World == nullptr || SenderController == nullptr)
	{
		return;
	}

	// 서버에서도 메시지의 앞뒤 공백을 다시 제거
	FString SanitizedMessage = MessageText;
	SanitizedMessage.TrimStartAndEndInline();

	// 비어 있거나 최대 길이를 초과한 메시지는 거부
	if (SanitizedMessage.IsEmpty() || SanitizedMessage.Len() > MaximumMessageLength)
	{
		return;
	}

	const double CurrentTime = World->GetTimeSeconds();

	// 플레이어가 너무 빠르게 연속 전송하면 메시지를 거부
	if (LastAcceptedMessageTime >= 0.0 && CurrentTime - LastAcceptedMessageTime < MinimumMessageInterval)
	{
		return;
	}

	APlayerState* SenderPlayerState = SenderController->PlayerState;

	// 서버에서 발신자 정보를 확인할 수 없으면 전송하지 않습니다.
	if (SenderPlayerState == nullptr)
	{
		return;
	}

	LastAcceptedMessageTime = CurrentTime;

	// 서버가 신뢰할 수 있는 정보로 최종 메시지를 생성
	FMultiplayerChatMessage ChatMessage;
	ChatMessage.MessageId = FGuid::NewGuid();
	ChatMessage.SenderId = FString::FromInt(SenderPlayerState->GetPlayerId());
	ChatMessage.SenderName = SenderPlayerState->GetPlayerName();
	ChatMessage.Channel = EMultiplayerChatChannel::Global;
	ChatMessage.ChannelId = TEXT("");
	ChatMessage.MessageText = SanitizedMessage;
	ChatMessage.ServerTimestamp = FDateTime::UtcNow();

	// 서버에 접속한 모든 플레이어의 채팅 컴포넌트를 찾습니다.
	for (FConstPlayerControllerIterator Iterator = 
		World->GetPlayerControllerIterator();
		 Iterator;
		 ++Iterator)
	{
		APlayerController* TargetController = Iterator->Get();

		if (TargetController == nullptr)
		{
			continue;
		}

		UMultiplayerChatComponent* TargetChatComponent = TargetController->FindComponentByClass<UMultiplayerChatComponent>();

		if (TargetChatComponent == nullptr)
		{
			continue;
		}

		// 각 PlayerController의 소유 클라이언트로 메시지를 전송
		TargetChatComponent->ClientReceiveMessage(ChatMessage);
	}
}

void UMultiplayerChatComponent::ClientReceiveMessage_Implementation(
	const FMultiplayerChatMessage& Message)
{
	// Blueprint와 UMG가 메시지를 표시할 수 있도록 이벤트를 발생.
	OnMessageReceived.Broadcast(Message);
}