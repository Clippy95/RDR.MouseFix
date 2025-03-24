// dllmain.cpp : Defines the entry point for the DLL application.
#define MINI_CASE_SENSITIVE
#include "pch.h"
#include "third_party/ModUtils/Patterns.h"
#include "third_party/ModUtils/MemoryMgr.h"

#include <windows.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <safetyhook.hpp>

#include "IniReader.h"
uintptr_t camera_pointer;

uintptr_t ReadPointer(uintptr_t baseAddress, const std::vector<uintptr_t>& offsets) {
    uintptr_t address = baseAddress;

    if (address == 0) {
        return 0;
    }

    for (size_t i = 0; i < offsets.size(); ++i) {
        uintptr_t* nextAddress = reinterpret_cast<uintptr_t*>(address);
        if (nextAddress == nullptr || *nextAddress == 0) {
            return 0;
        }
        address = *nextAddress + offsets[i];
    }

    return address;
}


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

SafetyHookMid cover_sens_hook;

uintptr_t sensaddr;
volatile float cover_sens_fix_multiplier = 2.f;
SAFETYHOOK_NOINLINE void cover_sens_MID(safetyhook::Context64& ctx) {
    ctx.xmm7.f32[0] *= cover_sens_fix_multiplier;

    uintptr_t* camera_ptr = (uintptr_t*)ctx.rax;
    //uintptr_t bool_ptr = ReadPointer(camera_ptr, { 0x28,0x620,0x30,0x8,0x2f0});
    printf("%p \n", camera_ptr);
}

void fix_halved_cover_sens() {
    auto pattern = hook::pattern("F3 0F 59 3D ? ? ? ? F3 0F 10 0D");
    cover_sens_hook = safetyhook::create_mid(pattern.get_first<void>(0), &cover_sens_MID);
    //pattern = hook::pattern("44 65 62 75 67 4B 65");
    //sensaddr = (uintptr_t)pattern.get_first<void>(0);
    //Memory::VP::Patch<float>(sensaddr, 1.2f);
}
SafetyHookMid cover_invert_fix;
SAFETYHOOK_NOINLINE void cover_invert_midfix(safetyhook::Context64& ctx) {
    float& y_delta = ctx.xmm8.f32[0];
    float& x_delta = ctx.xmm7.f32[0];

    //printf("x_delta %f y_delta %f \n", y_delta, x_delta);
    if (camera_pointer) {
        printf("camera_pointer %p \n", camera_pointer);
        uintptr_t bool_ptr = ReadPointer(camera_pointer, { 0x28,0x620,0x30,0x8 });
        bool_ptr = *(uintptr_t*)(bool_ptr);

        bool** invert_x = (bool**)(bool_ptr + 0x2f0);
        bool** invert_y = (bool**)(bool_ptr + 0x2f8);
        if (**invert_x)
            x_delta = -x_delta;
        if (**invert_y)
            y_delta = -y_delta;
    }
}

void fix_cover_invert() {
    auto pattern = hook::pattern("48 8B 05 ? ? ? ? F3 0F 10 05 ? ? ? ? 48 8B 48 ? F3 0F 10 89 ? ? ? ? E8 ? ? ? ? 8B 05 ? ? ? ? 0F 28 F0 F3 0F 59 35 ? ? ? ? F7 D8 66 0F 6E F8");
    if (!pattern.empty()) {
        uint32_t camera_ptr_offset = *pattern.get_first<uint32_t>(3);

        printf("camera offset %p\n", camera_ptr_offset);

        camera_pointer = (uintptr_t)pattern.get_first<uintptr_t>(7);
        camera_pointer += camera_ptr_offset;
    }
    // TBF I can put this in the cover_sens_MID and do bools but I'd rather be all seperated because what if the above fails?
    pattern = hook::pattern("E8 ? ? ? ? 44 8B C0 48 8D 55 ? 48 8B CB E8 ? ? ? ? 8B 18 81 E3 ? ? ? ? 74");
    if(!pattern.empty())
    cover_invert_fix = safetyhook::create_mid(pattern.get_first<void>(0),&cover_invert_midfix);
}
void fix_cover_clamping_mouse() {
    // Maybe use kananlib to to get "@GENERIC.TARGET" instead? this should suffice though imo.
    auto start = hook::pattern("F3 0F 5C C1 41 0F 2F C1 72 ? 0F 28 D7");
    auto jump = hook::pattern("F3 44 0F 58 87 ? ? ? ? F3 0F 10 0D");
    /*if (!start.empty() && !jump.empty()) {
        // E9 is quite big since it's literally quite short of a jump, but it should be fine.
        Memory::VP::InjectHook(start.get_first<void>(0), jump.get_first<void>(0), Memory::VP::HookType::Jump);
        printf("RDR.MouseFix pattern scan success start: %p jump: %p \n", start.get_first<void>(0), jump.get_first<void>(0));
    }
    else*/ if (!start.empty()) {
        // if jump doesn't work in future updates a nop should suffice.
        Memory::VP::Nop(start.get_first<void>(0), 0x2C);
        printf("RDR.MouseFix pattern scan success start nop: %p jump: %p \n", start.get_first<void>(0), 0x6969);
    }
    else
        printf("RDR.MouseFix pattern scan failed. \n");
}

void init() {
    //OpenConsole();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    CIniReader ini;
    bool fixMouseCoverSensitivity = ini.ReadBoolean("Fixes", "FixMouseCoverSensitivity", true);
    bool fixInvertOptionForMouseCover = ini.ReadBoolean("Fixes", "FixInvertOptionForMouseCover", false);
    if(ini.ReadFloat("Settings", "cover_sens_fix_multiplier", 2.0f) >= 0.1f)
    cover_sens_fix_multiplier = ini.ReadFloat("Settings", "cover_sens_fix_multiplier", 2.0f);
    fix_cover_clamping_mouse();
    if(fixMouseCoverSensitivity)
    fix_halved_cover_sens();
    if(fixInvertOptionForMouseCover)
    fix_cover_invert();
}

std::thread startitUp;
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {
        startitUp = std::thread(init);
        startitUp.detach();
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

