#include "TestActor.h"
#include "Engine/Engine.h"


ATestActor::ATestActor()
{
 	
	PrimaryActorTick.bCanEverTick = false;

}


void ATestActor::BeginPlay()
{
	Super::BeginPlay();
	
	// TestActor가 정상적으로 생성됐는지 Output Log에 출력한다.
	UE_LOG(LogTemp, Warning, TEXT("TestActor BeginPlay - Test module is working!"));
	
	
	// TestActor가 정상적으로 생성됐는지 게임 화면에 출력한다.
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.0f,
			FColor::Green,
			TEXT("Test module is working!"));
	}
	
}


void ATestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

