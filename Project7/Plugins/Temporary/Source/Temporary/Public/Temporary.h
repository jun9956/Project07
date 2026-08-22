#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// IModuleInterface: 모듈의 시작과 종료 기능을 정의하는 인터페이스
class FTemporaryModule : public IModuleInterface
{
public:
// StartupModule(): 플러그인 모듈이 로드될 때 실행
// ShutdownModule(): 플러그인 모듈이 종료될 때 실행
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
