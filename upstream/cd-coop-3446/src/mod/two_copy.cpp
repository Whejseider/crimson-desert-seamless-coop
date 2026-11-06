#include "mod/two_copy.h"

#include <windows.h>

#include <cstdint>
#include <thread>

#include "mod/log.h"

namespace cdmp::two_copy {
namespace {

// gdi32 (d3dkmthk.h): D3DKMTSetProcessSchedulingPriorityClass(HANDLE, D3DKMT_SCHEDULINGPRIORITYCLASS).
using SetPriorityFn = LONG(APIENTRY*)(HANDLE, int32_t);
constexpr int32_t kNormal = 2, kAboveNormal = 3, kHigh = 4;

bool foreground_is_ours() {
    DWORD pid = 0;
    if (const HWND fg = GetForegroundWindow()) GetWindowThreadProcessId(fg, &pid);
    return pid == GetCurrentProcessId();
}

}  // namespace

bool extra_process(const std::wstring& role) {
    const std::wstring name = L"Local\\cdmp_role_" + role;
    CreateMutexW(nullptr, FALSE, name.c_str());  // kept for the life of the process
    return GetLastError() == ERROR_ALREADY_EXISTS;
}

void keep_full_speed() {
    PROCESS_POWER_THROTTLING_STATE s{};
    s.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    s.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED | 0x4;  // 0x4: IGNORE_TIMER_RESOLUTION (Windows 11)
    s.StateMask = 0;
    const bool power = SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &s, sizeof s) != 0;
    const HMODULE gdi = GetModuleHandleW(L"gdi32.dll") ? GetModuleHandleW(L"gdi32.dll") : LoadLibraryW(L"gdi32.dll");
    const auto set = gdi ? reinterpret_cast<SetPriorityFn>(GetProcAddress(gdi, "D3DKMTSetProcessSchedulingPriorityClass"))
                         : nullptr;
    log::info("Two copies: power throttling {}; GPU priority switching {}", power ? "off" : "NOT changed",
              set ? "on" : "unavailable");
    if (!set) return;
    std::thread([set] {
        int32_t applied = kNormal;
        int failures = 0;
        for (;;) {
            Sleep(250);
            const int32_t want = foreground_is_ours() ? kNormal : kHigh;
            if (want == applied || failures >= 8) continue;
            LONG st = set(GetCurrentProcess(), want);
            int32_t got = want;
            if (st < 0 && want == kHigh) {
                st = set(GetCurrentProcess(), kAboveNormal);
                got = kAboveNormal;
            }
            if (st < 0) {
                ++failures;
                continue;
            }
            applied = got;
            log::info("Two copies: GPU priority {} (the focus is {})", got == kNormal ? "normal" : got == kHigh ? "high" : "above normal",
                      got == kNormal ? "ours" : "elsewhere");
        }
    }).detach();
}

}  // namespace cdmp::two_copy
