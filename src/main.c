#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tlhelp32.h>
#include <wchar.h>

// Logical Core 4 (5th hardware thread, bitmask 0x10 / decimal 16)
#define PREFERRED_AUDIODG_AFFINITY ((DWORD_PTR)1 << 4)

// Detected VoiceMeeter editions
typedef enum {
    VM_NONE         = -1,
    VM_STANDARD_X86 = 0,
    VM_STANDARD_X64 = 1,
    VM_BANANA_X86   = 2,
    VM_BANANA_X64   = 3,
    VM_POTATO_X86   = 4,
    VM_POTATO_X64   = 5
} VoiceMeeterEdition;

// Enable SeDebugPrivilege to allow modifying protected/system audio processes like audiodg.exe
static BOOL EnableDebugPrivileges(void) {
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tkp;
    BOOL success = FALSE;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        ZeroMemory(&tkp, sizeof(tkp));
        if (LookupPrivilegeValueW(NULL, L"SeDebugPrivilege", &tkp.Privileges[0].Luid)) {
            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            // AdjustTokenPrivileges returns TRUE even when privileges are not assigned; verify via GetLastError()
            SetLastError(ERROR_SUCCESS);
            if (AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL)) {
                success = (GetLastError() == ERROR_SUCCESS);
            }
        }
        CloseHandle(hToken);
    }
    return success;
}

// Match an executable filename against all 32-bit and 64-bit VoiceMeeter editions
static VoiceMeeterEdition DetectVoiceMeeterEdition(const WCHAR* exeName) {
    static const WCHAR* const vmTargets[] = {
        L"voicemeeter.exe",        // 0: VM_STANDARD_X86
        L"voicemeeter_x64.exe",    // 1: VM_STANDARD_X64
        L"voicemeeterpro.exe",     // 2: VM_BANANA_X86
        L"voicemeeterpro_x64.exe", // 3: VM_BANANA_X64
        L"voicemeeter8.exe",       // 4: VM_POTATO_X86
        L"voicemeeter8x64.exe"     // 5: VM_POTATO_X64 (VB-Audio omits underscore here)
    };

    const int targetCount = (int)(sizeof(vmTargets) / sizeof(vmTargets[0]));
    for (int i = 0; i < targetCount; ++i) {
        if (_wcsicmp(exeName, vmTargets[i]) == 0) {
            return (VoiceMeeterEdition)i;
        }
    }
    return VM_NONE;
}

// Select Logical Core 4 (5th thread) if present; otherwise fall back to the highest valid CPU core
static DWORD_PTR ResolveSafeAffinity(HANDLE hProcess, DWORD_PTR preferredMask) {
    DWORD_PTR procMask = 0;
    DWORD_PTR sysMask = 0;

    if (!GetProcessAffinityMask(hProcess, &procMask, &sysMask) || sysMask == 0) {
        return 0; // Leave affinity unchanged if query fails
    }
    if ((sysMask & preferredMask) != 0) {
        return preferredMask; // 5+ threads exist: use Logical Core 4 (mask 16)
    }

    // Fallback for <= 4-thread CPUs: pick the highest valid core away from Core 0
    DWORD_PTR bit = (DWORD_PTR)1 << (sizeof(DWORD_PTR) * 8 - 1);
    while (bit > 0 && (sysMask & bit) == 0) {
        bit >>= 1;
    }
    return bit;
}

// Open target process by PID and apply CPU priority class + optional affinity mask
static void ApplySettingsToPid(DWORD pid, DWORD priorityClass, DWORD_PTR preferredAffinity) {
    DWORD accessRights = PROCESS_SET_INFORMATION;
    if (preferredAffinity != 0) {
        // PROCESS_QUERY_LIMITED_INFORMATION is only required when querying system affinity masks
        accessRights |= PROCESS_QUERY_LIMITED_INFORMATION;
    }

    HANDLE hProcess = OpenProcess(accessRights, FALSE, pid);
    if (hProcess != NULL) {
        SetPriorityClass(hProcess, priorityClass);

        if (preferredAffinity != 0) {
            DWORD_PTR safeMask = ResolveSafeAffinity(hProcess, preferredAffinity);
            if (safeMask != 0) {
                SetProcessAffinityMask(hProcess, safeMask);
            }
        }
        CloseHandle(hProcess);
    }
}

// Single-snapshot scan for audiodg.exe and all running VoiceMeeter editions
static void RunAudioOptimizations(void) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        return;
    }

    PROCESSENTRY32W pe;
    ZeroMemory(&pe, sizeof(pe));
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(hSnap, &pe)) {
        do {
            // 1. audiodg.exe: High priority + isolated single-core affinity
            if (_wcsicmp(pe.szExeFile, L"audiodg.exe") == 0) {
                ApplySettingsToPid(pe.th32ProcessID, HIGH_PRIORITY_CLASS, PREFERRED_AUDIODG_AFFINITY);
            }
            // 2. VoiceMeeter (Standard, Banana, Potato - x86/x64): Above Normal priority + affinity unchanged
            else if (DetectVoiceMeeterEdition(pe.szExeFile) != VM_NONE) {
                ApplySettingsToPid(pe.th32ProcessID, ABOVE_NORMAL_PRIORITY_CLASS, 0);
            }
        } while (Process32NextW(hSnap, &pe));
    }

    CloseHandle(hSnap);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // 0. Prevent overlapping instances across all sessions during the 118-second window
    HANDLE hMutex = CreateMutexW(NULL, FALSE, L"Global\\VBAudioDSPDropoutFix_SingleInstance");
    DWORD mutexErr = GetLastError();
    if (hMutex == NULL) {
        // ERROR_ACCESS_DENIED indicates an elevated/SYSTEM instance already owns the Global mutex
        return (mutexErr == ERROR_ACCESS_DENIED) ? 0 : 1;
    }
    if (mutexErr == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMutex);
        return 0; // Another instance is already executing the optimization sequence
    }

    // 1. Acquire debug privileges to modify Windows Audio Device Graph Isolation (audiodg.exe)
    EnableDebugPrivileges();

    // 2. Execute the 6-pass, 118-second startup/wake optimization sequence
    static const DWORD sleepIntervals[] = { 24000, 24000, 24000, 24000, 22000 };
    const size_t numIntervals = sizeof(sleepIntervals) / sizeof(sleepIntervals[0]);

    for (size_t i = 0; i < numIntervals; ++i) {
        RunAudioOptimizations();
        Sleep(sleepIntervals[i]);
    }
    RunAudioOptimizations(); // Final (6th) pass at t = 118s

    // 3. Release single-instance mutex before exit
    CloseHandle(hMutex);

    return 0;
}