#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TemporaryActor.generated.h"

UCLASS()
class TEMPORARY_API ATemporaryActor : public AActor
{
	GENERATED_BODY()
	
public:	
	
	ATemporaryActor();

protected:
	
	virtual void BeginPlay() override;

public:	
	
	virtual void Tick(float DeltaTime) override;

};
