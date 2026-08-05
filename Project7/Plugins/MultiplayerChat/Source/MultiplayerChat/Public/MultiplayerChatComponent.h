#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Components/ActorComponent.h"
#include "MultiplayerChatTypes.h"
#include "MultiplayerChatComponent.generated.h"

class UUserWidget;
class UInputComponent;
class APlayerState;

// 클라이언트가 채팅 메시지를 수신했을 때 발생하는 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnMultiplayerChatMessageReceived,
	const FMultiplayerChatMessage&,
	Message
);

// 채팅 입력 활성 상태가 변경될 때 발생하는 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnMultiplayerChatInputStateChanged,
	bool,
	bIsActive
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

	// 채팅 입력을 활성화하고 Game And UI 입력 모드로 변경
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Chat|Input")
	void ActivateChatInput();

	// 채팅 입력을 종료하고 Game Only 입력 모드로 복귀
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Chat|Input")
	void DeactivateChatInput();

	// 채팅 입력 상태가 변경될 때 UI에 알리는 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Multiplayer Chat|Input")
	FOnMultiplayerChatInputStateChanged OnChatInputStateChanged;

protected:
	// 컴포넌트가 게임에 진입하면 로컬 플레이어의 채팅 UI를 생성
	virtual void BeginPlay() override;

	// 게임이 종료되거나 컴포넌트가 제거될 때 채팅 UI를 정리
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
private:
	
	// 클라이언트가 서버로 메시지를 보낼 때 사용하는 RPC
	UFUNCTION(Server, Reliable)
	void ServerSendGlobalMessage(const FString& MessageText);
	
	// 서버가 특정 클라이언트에 메시지를 전달할 때 사용하는 RPC
	UFUNCTION(Client, Reliable)
	void ClientReceiveMessage(const FMultiplayerChatMessage& Message);

	// 서버에서 도배 방지를 계산하기 위해 사용하는 마지막 전송 시각
	double LastAcceptedMessageTime = -1.0;
	
	// 컴포넌트가 기본 채팅 UI를 자동 생성할지 결정
	UPROPERTY(EditDefaultsOnly, Category = "Multiplayer Chat|UI")
	bool bAutoCreateChatWidget = true;

	// 화면에 생성할 기본 채팅 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Multiplayer Chat|UI")
	TSubclassOf<UUserWidget> ChatWidgetClass;

	// 현재 로컬 플레이어 화면에 생성된 채팅 위젯
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ChatWidget;

	// 현재 로컬 플레이어가 채팅을 입력 중인지 나타냅니다.
	bool bIsChatInputActive = false;

	// 로컬 플레이어용 채팅 입력을 생성하고 입력 스택에 등록
	void SetupLocalChatInput();

	// 생성한 채팅 입력을 입력 스택에서 제거하고 정리
	void TeardownLocalChatInput();

	// 자동 바인딩된 활성화 키를 처리
	void HandleActivateChatInput();

	// 자동 바인딩된 취소 키를 처리
	void HandleCancelChatInput();

	// 플러그인이 채팅 입력 키를 자동으로 등록할지 결정
	UPROPERTY(EditDefaultsOnly, Category = "Multiplayer Chat|Input")
	bool bAutoBindChatInput = true;

	// 채팅 입력을 활성화하는 키
	UPROPERTY(EditDefaultsOnly, Category = "Multiplayer Chat|Input")
	FKey ActivateChatKey = EKeys::Enter;

	// 채팅 입력을 취소하는 키
	UPROPERTY(EditDefaultsOnly, Category = "Multiplayer Chat|Input")
	FKey CancelChatKey = EKeys::Escape;

	// 다른 입력 컴포넌트보다 먼저 채팅 키를 처리하기 위한 우선순위
	UPROPERTY(EditDefaultsOnly, Category = "Multiplayer Chat|Input")
	int32 ChatInputPriority = 100;

	// 로컬 플레이어의 채팅 키 바인딩을 보관하는 입력 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UInputComponent> ChatInputComponent;

	// Escape 바인딩의 소비 설정을 변경하기 위한 인덱스
	int32 CancelInputBindingIndex = INDEX_NONE;

	// 프로젝트 Identity Provider 또는 PlayerState에서 플레이어 ID를 결정
	FString ResolvePlayerId(APlayerState* PlayerState) const;

	// 프로젝트 Identity Provider 또는 PlayerState에서 표시 이름을 결정
	FString ResolveDisplayName(APlayerState* PlayerState) const;
};
