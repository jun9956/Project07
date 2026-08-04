#include "TemporaryActor.h"


ATemporaryActor::ATemporaryActor()
{
 	
	PrimaryActorTick.bCanEverTick = false;

}


void ATemporaryActor::BeginPlay()
{
	Super::BeginPlay();
	
}


void ATemporaryActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

