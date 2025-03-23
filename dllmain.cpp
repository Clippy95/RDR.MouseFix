// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "third_party/ModUtils/Patterns.h"
#include "third_party/ModUtils/MemoryMgr.h"

#include <windows.h>
#include <iostream>
#include <fstream>
#if DEBUG
void OpenConsole()
{
    AllocConsole();

    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);

    SetConsoleTitleA("Debug Console");

    std::cout << "[+] Console allocated!" << std::endl;
}
#endif

// Removes mouse clamping while in cover.
void fix_cover_mouse() {
    auto start = hook::pattern("F3 0F 5C C1 41 0F 2F C1 72 ? 0F 28 D7");
    auto jump = hook::pattern("F3 44 0F 58 87 ? ? ? ? F3 0F 10 0D");
    if (!start.empty() && !jump.empty())
        Memory::VP::InjectHook(start.get_first<void>(0), jump.get_first<void>(0), Memory::VP::HookType::Jump);
    else
        printf("RDR.MouseFix pattern scan failed.");
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        fix_cover_mouse();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

