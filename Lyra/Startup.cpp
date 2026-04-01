#include "./Lyra.h"

#include <windows.h>
#include <cstdint>

int32_t wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {
    Lyra::Initialize(hInstance);
    auto test = std::make_shared<Lyra::WindowFoundation>();

    test->Create(L"Test Window");
    test->Activate();
    Lyra::Run(test);
    Lyra::Uninitialize();
    return NULL;
}