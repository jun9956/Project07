#include "Temporary.h"

void FTemporaryModule::StartupModule()
{
    // 플러그인 모듈이 로드될 때 로그를 출력한다.
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Temporary plugin module started.")
    );
}

void FTemporaryModule::ShutdownModule()
{
    // 플러그인 모듈이 종료될 때 로그를 출력한다.
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Temporary plugin module shut down.")
    );
}

// FTemporaryModule을 Temporary 모듈의 구현체로 등록한다.
IMPLEMENT_MODULE(FTemporaryModule, Temporary);
