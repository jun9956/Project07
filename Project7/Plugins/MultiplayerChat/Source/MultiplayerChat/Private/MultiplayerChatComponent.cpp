#include "MultiplayerChatComponent.h"

#include "MultiplayerChatIdentityProvider.h"
#include "Components/InputComponent.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogMultiplayerChat, Log, All);

UMultiplayerChatComponent::UMultiplayerChatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// 플러그인에 포함된 기본 채팅 위젯 클래스를 설정
	static ConstructorHelpers::FClassFinder<UUserWidget>DefaultChatWidgetClass(TEXT("/MultiplayerChat/UI/WBP_MultiplayerChat"));

	if (DefaultChatWidgetClass.Succeeded())
	{
		ChatWidgetClass = DefaultChatWidgetClass.Class;
	}

	ReservedNicknames =
	{
		TEXT("System"),
		TEXT("Server"),
		TEXT("Admin")
	};

	// Server RPC와 Client RPC를 사용할 수 있도록 컴포넌트 복제를 활성화
	SetIsReplicatedByDefault(true);
}

void UMultiplayerChatComponent::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* OwningPlayerController = Cast<APlayerController>(GetOwner());

	// 로컬 PlayerController에서만 UI와 입력을 준비합니다.
	if (OwningPlayerController == nullptr || !OwningPlayerController->IsLocalController())
	{
		return;
	}

	if (bAutoCreateChatWidget && ChatWidgetClass != nullptr)
	{
		ChatWidget = CreateWidget<UUserWidget>(OwningPlayerController,ChatWidgetClass);

		if (ChatWidget != nullptr)
		{
			ChatWidget->AddToPlayerScreen();
		}
	}

	SetupLocalChatInput();
}

void UMultiplayerChatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bIsChatInputActive)
	{
		DeactivateChatInput();
	}

	TeardownLocalChatInput();

	// 생성된 채팅 UI가 있으면 화면에서 제거
	if (ChatWidget != nullptr)
	{
		ChatWidget->RemoveFromParent();
		ChatWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
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

void UMultiplayerChatComponent::ActivateChatInput()
{
	if (bIsChatInputActive)
	{
		return;
	}

	APlayerController* OwningPlayerController = Cast<APlayerController>(GetOwner());

	if (OwningPlayerController == nullptr || !OwningPlayerController->IsLocalController())
	{
		return;
	}

	bIsChatInputActive = true;

	if (ChatInputComponent != nullptr && ChatInputComponent->KeyBindings.IsValidIndex(CancelInputBindingIndex))
	{
		// 채팅 중에는 Escape가 게임의 일시정지 등에 전달되지 않게 합니다.
		ChatInputComponent->KeyBindings[CancelInputBindingIndex].bConsumeInput = true;
	}

	FInputModeGameAndUI InputMode;
	OwningPlayerController->SetInputMode(InputMode);

	// Blueprint UI가 실제 입력 위젯에 포커스를 줄 수 있도록 알립니다.
	OnChatInputStateChanged.Broadcast(true);
}

void UMultiplayerChatComponent::DeactivateChatInput()
{
	if (!bIsChatInputActive)
	{
		return;
	}

	// 포커스 변경 과정에서 재호출되어도 안전하도록 먼저 상태를 변경합니다.
	bIsChatInputActive = false;

	if (ChatInputComponent != nullptr && ChatInputComponent->KeyBindings.IsValidIndex(CancelInputBindingIndex))
	{
		// 채팅이 끝나면 Escape를 다시 다른 게임 입력에 전달합니다.
		ChatInputComponent->KeyBindings[CancelInputBindingIndex].bConsumeInput = false;
	}

	APlayerController* OwningPlayerController = Cast<APlayerController>(GetOwner());

	if (OwningPlayerController != nullptr && OwningPlayerController->IsLocalController())
	{
		FInputModeGameOnly InputMode;
		OwningPlayerController->SetInputMode(InputMode);
	}

	OnChatInputStateChanged.Broadcast(false);
}

void UMultiplayerChatComponent::RequestNicknameChange(const FString& NewNickname)
{
	APlayerController* OwningPlayerController = Cast<APlayerController>(GetOwner());

	// 소유 로컬 플레이어만 닉네임 변경을 요청할 수 있습니다.
	if (OwningPlayerController == nullptr || !OwningPlayerController->IsLocalController())
	{
		return;
	}

	FString SanitizedNickname = NewNickname;
	SanitizedNickname.TrimStartAndEndInline();

	ServerRequestNicknameChange(SanitizedNickname);
}

void UMultiplayerChatComponent::ServerSendGlobalMessage_Implementation(const FString& MessageText)
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
	ChatMessage.SenderId = ResolvePlayerId(SenderPlayerState);
	ChatMessage.SenderName = ResolveDisplayName(SenderPlayerState);
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

void UMultiplayerChatComponent::ClientReceiveMessage_Implementation(const FMultiplayerChatMessage& Message)
{
	// UI가 없는 상태에서도 수신 결과를 확인할 수 있도록 로그를 출력
	UE_LOG(
		LogMultiplayerChat,
		Log,
		TEXT("[%s] %s: %s"),
		*UEnum::GetValueAsString(Message.Channel),
		*Message.SenderName,
		*Message.MessageText
	);

	// Blueprint와 UMG가 메시지를 표시할 수 있도록 이벤트를 발생
	OnMessageReceived.Broadcast(Message);
}

void UMultiplayerChatComponent::SetupLocalChatInput()
{
	if (!bAutoBindChatInput || ChatInputComponent != nullptr)
	{
		return;
	}

	APlayerController* OwningPlayerController = Cast<APlayerController>(GetOwner());

	if (OwningPlayerController == nullptr || !OwningPlayerController->IsLocalController())
	{
		return;
	}

	ChatInputComponent = NewObject<UInputComponent>(OwningPlayerController,TEXT("MultiplayerChatInputComponent"));

	if (ChatInputComponent == nullptr)
	{
		return;
	}

	ChatInputComponent->RegisterComponent();
	ChatInputComponent->Priority = ChatInputPriority;
	ChatInputComponent->bBlockInput = false;

	FInputKeyBinding& ActivateBinding =
		ChatInputComponent->BindKey(
			ActivateChatKey,
			IE_Pressed,
			this,
			&UMultiplayerChatComponent::HandleActivateChatInput
		);

	// 활성화 키는 다른 게임 입력으로 전달하지 않습니다.
	ActivateBinding.bConsumeInput = true;

	FInputKeyBinding& CancelBinding =
		ChatInputComponent->BindKey(
			CancelChatKey,
			IE_Pressed,
			this,
			&UMultiplayerChatComponent::HandleCancelChatInput
		);

	// 채팅 중이 아닐 때는 Escape를 다른 게임 입력으로 전달
	CancelBinding.bConsumeInput = false;
	CancelInputBindingIndex = ChatInputComponent->KeyBindings.Num() - 1;

	OwningPlayerController->PushInputComponent(ChatInputComponent);
}

void UMultiplayerChatComponent::TeardownLocalChatInput()
{
	if (ChatInputComponent == nullptr)
	{
		return;
	}

	APlayerController* OwningPlayerController = Cast<APlayerController>(GetOwner());

	if (OwningPlayerController != nullptr)
	{
		OwningPlayerController->PopInputComponent(ChatInputComponent);
	}

	ChatInputComponent->DestroyComponent();
	ChatInputComponent = nullptr;
	CancelInputBindingIndex = INDEX_NONE;
}

void UMultiplayerChatComponent::HandleActivateChatInput()
{
	ActivateChatInput();
}

void UMultiplayerChatComponent::HandleCancelChatInput()
{
	DeactivateChatInput();
}

// 플레이어 ID 결정 함수
FString UMultiplayerChatComponent::ResolvePlayerId(APlayerState* PlayerState) const
{
	if (PlayerState == nullptr)
	{
		return FString();
	}

	const bool bHasIdentityProvider = PlayerState->GetClass()->ImplementsInterface(UMultiplayerChatIdentityProvider::StaticClass());

	if (bHasIdentityProvider)
	{
		FString ProviderPlayerId = IMultiplayerChatIdentityProvider::Execute_GetMultiplayerChatPlayerId(PlayerState);

		ProviderPlayerId.TrimStartAndEndInline();

		if (!ProviderPlayerId.IsEmpty())
		{
			return ProviderPlayerId;
		}
	}

	const FUniqueNetIdRepl& UniqueId = PlayerState->GetUniqueId();

	if (UniqueId.IsValid())
	{
		return UniqueId.ToString();
	}

	// Online Subsystem이 없는 PIE와 LAN 환경에서는 세션 PlayerId를 사용합니다.
	return FString::FromInt(PlayerState->GetPlayerId());
}

// 표시 이름 결정 함수
FString UMultiplayerChatComponent::ResolveDisplayName(APlayerState* PlayerState) const
{
	if (PlayerState == nullptr)
	{
		return FString();
	}
	const bool bHasIdentityProvider = PlayerState->GetClass()->ImplementsInterface(UMultiplayerChatIdentityProvider::StaticClass());

	if (bHasIdentityProvider)
	{
		FString ProviderDisplayName = IMultiplayerChatIdentityProvider::Execute_GetMultiplayerChatDisplayName(PlayerState);

		ProviderDisplayName.TrimStartAndEndInline();

		if (!ProviderDisplayName.IsEmpty())
		{
			return ProviderDisplayName;
		}
	}

	// Identity Provider가 없거나 빈 이름을 반환하면 기본 PlayerState 이름을 사용합니다.
	return PlayerState->GetPlayerName();
}

void UMultiplayerChatComponent::ServerRequestNicknameChange_Implementation(const FString& NewNickname)
{
	FString SanitizedNickname = NewNickname;
	SanitizedNickname.TrimStartAndEndInline();

	APlayerController* OwningPlayerController =Cast<APlayerController>(GetOwner());

	APlayerState* SenderPlayerState = OwningPlayerController != nullptr? OwningPlayerController->PlayerState: nullptr;

	UWorld* World = GetWorld();

	if (OwningPlayerController == nullptr || SenderPlayerState == nullptr || World == nullptr)
	{
		ClientReceiveNicknameChangeResult(EMultiplayerChatNicknameResult::Unavailable,SanitizedNickname);
		return;
	}

	if (!bAllowClientNicknameChanges)
	{
		ClientReceiveNicknameChangeResult(EMultiplayerChatNicknameResult::ChangesDisabled,SanitizedNickname);
		return;
	}

	// 외부 Identity Provider가 이름을 제공하면 플러그인이 변경하지 않습니다.
	const bool bHasIdentityProvider =SenderPlayerState->GetClass()->ImplementsInterface(UMultiplayerChatIdentityProvider::StaticClass());

	if (bHasIdentityProvider)
	{
		FString ProviderDisplayName =IMultiplayerChatIdentityProvider::Execute_GetMultiplayerChatDisplayName(SenderPlayerState);

		ProviderDisplayName.TrimStartAndEndInline();

		if (!ProviderDisplayName.IsEmpty())
		{
			ClientReceiveNicknameChangeResult(EMultiplayerChatNicknameResult::ManagedExternally,SanitizedNickname);
			return;
		}
	}

	const double CurrentTime = World->GetTimeSeconds();

	// 너무 빠른 닉네임 변경 요청을 거부합니다.
	if (LastNicknameChangeRequestTime >= 0.0 && CurrentTime - LastNicknameChangeRequestTime< MinimumNicknameChangeInterval)
	{
		ClientReceiveNicknameChangeResult(EMultiplayerChatNicknameResult::RateLimited,SanitizedNickname);
		return;
	}

	LastNicknameChangeRequestTime = CurrentTime;

	if (SanitizedNickname.Len() < MinimumNicknameLength || SanitizedNickname.Len() > MaximumNicknameLength)
	{
		ClientReceiveNicknameChangeResult(EMultiplayerChatNicknameResult::InvalidLength,SanitizedNickname);
		return;
	}

	// 한글, 영문, 숫자와 밑줄만 허용합니다.
	for (const TCHAR Character : SanitizedNickname)
	{
		const bool bIsHangul =
			(Character >= 0x1100 && Character <= 0x11FF)
			|| (Character >= 0x3130 && Character <= 0x318F)
			|| (Character >= 0xAC00 && Character <= 0xD7A3);

		const bool bIsAllowed =
			FChar::IsAlnum(Character) || bIsHangul || Character == TEXT('_');

		if (!bIsAllowed)
		{
			ClientReceiveNicknameChangeResult(EMultiplayerChatNicknameResult::InvalidCharacters,SanitizedNickname);
			return;
		}
	}

	for (const FString& ReservedNickname : ReservedNicknames)
	{
		if (ReservedNickname.Equals(SanitizedNickname,ESearchCase::IgnoreCase))
		{
			ClientReceiveNicknameChangeResult(EMultiplayerChatNicknameResult::ReservedName,SanitizedNickname);
			return;
		}
	}

	// 다른 접속 플레이어가 동일한 닉네임을 사용하는지 확인합니다.
	for (FConstPlayerControllerIterator Iterator =
			World->GetPlayerControllerIterator();
		 Iterator;
		 ++Iterator)
	{
		APlayerController* TargetController = Iterator->Get();

		if (TargetController == nullptr || TargetController == OwningPlayerController || TargetController->PlayerState == nullptr)
		{
			continue;
		}

		const FString ExistingDisplayName =ResolveDisplayName(TargetController->PlayerState);

		if (ExistingDisplayName.Equals(SanitizedNickname,ESearchCase::IgnoreCase))
		{
			ClientReceiveNicknameChangeResult(EMultiplayerChatNicknameResult::AlreadyInUse,SanitizedNickname);
			return;
		}
	}

	// 검증에 통과한 이름을 서버의 PlayerState에 적용합니다.
	SenderPlayerState->SetPlayerName(SanitizedNickname);
	SenderPlayerState->ForceNetUpdate();

	ClientReceiveNicknameChangeResult(EMultiplayerChatNicknameResult::Success,SanitizedNickname);
}

void UMultiplayerChatComponent::ClientReceiveNicknameChangeResult_Implementation(EMultiplayerChatNicknameResult Result,const FString& Nickname)
{
	OnNicknameChangeResult.Broadcast(Result, Nickname);
}
