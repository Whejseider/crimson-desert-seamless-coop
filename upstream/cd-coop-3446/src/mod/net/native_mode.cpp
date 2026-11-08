#include "mod/net/native_mode.h"

#include <windows.h>

#include <string>
#include <vector>

#include "mod/log.h"
#include "mod/native_layer.h"

namespace cdmp::net {
namespace {

std::string narrow_path(const std::wstring& w) {
    if (w.empty()) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<size_t>(n > 0 ? n : 0), '\0');
    if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), s.data(), n, nullptr, nullptr);
    return s;
}

std::wstring widen(const std::string& s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<size_t>(n > 0 ? n : 0), L'\0');
    if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}

// The file as text: UTF-16 (with its BOM) or 8-bit; empty when absent.
std::string read_text(const std::wstring& path) {
    HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0,
                           nullptr);
    if (f == INVALID_HANDLE_VALUE) return {};
    std::vector<char> b;
    LARGE_INTEGER size{};
    if (GetFileSizeEx(f, &size) && size.QuadPart > 0 && size.QuadPart < (1 << 20)) {
        b.resize(static_cast<size_t>(size.QuadPart));
        DWORD got = 0;
        if (!ReadFile(f, b.data(), static_cast<DWORD>(b.size()), &got, nullptr)) got = 0;
        b.resize(got);
    }
    CloseHandle(f);
    if (b.size() >= 2 && static_cast<unsigned char>(b[0]) == 0xFF && static_cast<unsigned char>(b[1]) == 0xFE) {
        std::wstring w(reinterpret_cast<const wchar_t*>(b.data() + 2), (b.size() - 2) / 2);
        return narrow_path(w);
    }
    return std::string(b.begin(), b.end());
}

}  // namespace

std::wstring native_mode_ini_path(const std::wstring& bin_dir) {
    const std::wstring role = test_role();
    return bin_dir + (role.empty() ? L"cdmp.ini" : L"cdmp." + role + L".ini");
}

NativeMode native_mode_current(const std::wstring& bin_dir) {
    return native_mode_of_text(read_text(native_mode_ini_path(bin_dir)));
}

bool native_mode_write(const std::wstring& bin_dir, const NativeModeSpec& spec) {
    const std::wstring path = native_mode_ini_path(bin_dir);
    const std::string before = read_text(path);
    if (!before.empty() && before.find("; coopplus_mode=") == std::string::npos) {
        const std::wstring keep = path + L".before-coopplus";
        if (GetFileAttributesW(keep.c_str()) == INVALID_FILE_ATTRIBUTES && !CopyFileW(path.c_str(), keep.c_str(), TRUE)) {
            log::warn("Native mode: could not keep the earlier '{}' (error {}); not rewritten", narrow_path(path),
                      GetLastError());
            return false;
        }
    }
    const std::wstring w = widen(native_mode_ini_text(spec));
    const std::wstring tmp = path + L".tmp";
    HANDLE f = CreateFileW(tmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) {
        log::warn("Native mode: cannot write '{}' (error {})", narrow_path(tmp), GetLastError());
        return false;
    }
    const unsigned char bom[2] = {0xFF, 0xFE};
    DWORD put = 0, put2 = 0;
    const bool ok = WriteFile(f, bom, 2, &put, nullptr) &&
                    WriteFile(f, w.data(), static_cast<DWORD>(w.size() * sizeof(wchar_t)), &put2, nullptr) &&
                    put2 == w.size() * sizeof(wchar_t);
    CloseHandle(f);
    if (!ok || !MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        log::warn("Native mode: cannot replace '{}' (error {})", narrow_path(path), GetLastError());
        DeleteFileW(tmp.c_str());
        return false;
    }
    log::info("Native mode: '{}' now says {} ({}{}{})", narrow_path(path), native_mode_name(spec.mode),
              spec.mode == NativeMode::Solo ? "the player's own game" : (spec.steam ? "Steam" : "direct"),
              spec.coop_save_dir.empty() ? "" : ", co-op save " + spec.coop_save_dir,
              spec.coop_save_dir.empty() ? "" : " (" + spec.coop_save_seed + ")");
    return true;
}

bool native_restart_game(const char* why) {
    wchar_t exe[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    const DWORD pid = GetCurrentProcessId();
    // With CDMP_ROLE (two copies on one PC) Steam would refuse a second copy: the exe is started the way the
    // harness starts it, inheriting this process's environment (role, sandbox variables, SteamAppId).
    const bool direct = !test_role().empty();
    std::wstring start = direct ? L"Start-Process -FilePath '" + std::wstring(exe) +
                                      L"' -ArgumentList 'PlatformServiceType=Steam' -WorkingDirectory '" +
                                      std::wstring(exe).substr(0, std::wstring(exe).find_last_of(L'\\')) + L"'"
                                : L"Start-Process 'steam://rungameid/3321460'";
    // The helper writes what it did next to the game (coopplus.restart.log): a restart that never came back says why.
    const std::wstring bin = std::wstring(exe).substr(0, std::wstring(exe).find_last_of(L'\\'));
    const std::wstring note = L"'" + bin + L"\\coopplus.restart.log'";
    const std::wstring cmd =
        L"powershell.exe -NoProfile -NonInteractive -WindowStyle Hidden -Command \"try { (Get-Date).ToString('o') + "
        L"' waiting for " + std::to_wstring(pid) + L"' | Out-File -Append " + note + L"; Wait-Process -Id " +
        std::to_wstring(pid) + L" -Timeout 120 -ErrorAction SilentlyContinue; Start-Sleep -Seconds 3; " + start +
        L"; (Get-Date).ToString('o') + ' started the game' | Out-File -Append " + note +
        L" } catch { (Get-Date).ToString('o') + ' FAILED: ' + $_ | Out-File -Append " + note + L" }\"";
    std::vector<wchar_t> line(cmd.begin(), cmd.end());
    line.push_back(L'\0');
    STARTUPINFOW si{};
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    BOOL in_job = FALSE;
    IsProcessInJob(GetCurrentProcess(), nullptr, &in_job);
    // tcB5: the game may run in a job that ends its children with it: the helper breaks away when the job allows it.
    DWORD flags = CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP | CREATE_BREAKAWAY_FROM_JOB;
    bool ok = CreateProcessW(nullptr, line.data(), nullptr, nullptr, FALSE, flags, nullptr, nullptr, &si, &pi) != 0;
    if (!ok) {
        flags &= ~static_cast<DWORD>(CREATE_BREAKAWAY_FROM_JOB);
        ok = CreateProcessW(nullptr, line.data(), nullptr, nullptr, FALSE, flags, nullptr, nullptr, &si, &pi) != 0;
    }
    if (!ok) {
        log::warn("Native mode: the restart helper did not start (error {}); the game keeps running", GetLastError());
        return false;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    log::info("Native mode: restarting the game ({}){}; in a job: {}, helper {}", why,
              direct ? " -- two copies: the exe directly" : "", in_job ? "yes" : "no",
              flags & CREATE_BREAKAWAY_FROM_JOB ? "broke away" : "in the job");
    log::close();  // flushed before the process ends
    TerminateProcess(GetCurrentProcess(), 0);
    return true;
}

}  // namespace cdmp::net

namespace cdmp::net {
namespace {
constexpr uint64_t kResumeFreshS = 15 * 60;
constexpr const wchar_t* kSec = L"native_resume";
uint64_t unix_now() {
    FILETIME ft{};
    GetSystemTimeAsFileTime(&ft);
    const uint64_t t = (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    return t / 10000000ull - 11644473600ull;
}
std::wstring get(const std::wstring& ini, const wchar_t* key) {
    wchar_t buf[256] = {};
    GetPrivateProfileStringW(kSec, key, L"", buf, 256, ini.c_str());
    return buf;
}
}  // namespace

NativeResume native_resume_read(const std::wstring& ini_path) {
    NativeResume r;
    const std::wstring m = get(ini_path, L"mode");
    r.mode = m == L"host" ? NativeMode::Host : m == L"guest" ? NativeMode::Guest : NativeMode::Solo;
    r.steam = get(ini_path, L"transport") != L"udp";
    r.partner_steam_id = _wcstoui64(get(ini_path, L"partner_steamid").c_str(), nullptr, 10);
    r.partner_address = narrow_path(get(ini_path, L"partner_address"));
    r.plan = static_cast<uint8_t>(_wtoi(get(ini_path, L"plan").c_str()));
    r.written_unix = _wcstoui64(get(ini_path, L"written").c_str(), nullptr, 10);
    return r;
}

void native_resume_write(const std::wstring& ini_path, NativeResume r) {
    r.written_unix = unix_now();
    const auto put = [&](const wchar_t* k, const std::wstring& v) {
        WritePrivateProfileStringW(kSec, k, v.c_str(), ini_path.c_str());
    };
    put(L"mode", r.mode == NativeMode::Host ? L"host" : r.mode == NativeMode::Guest ? L"guest" : L"solo");
    put(L"transport", r.steam ? L"steam" : L"udp");
    put(L"partner_steamid", r.partner_steam_id ? std::to_wstring(r.partner_steam_id) : L"");
    put(L"partner_address", widen(native_ini_value(r.partner_address)));
    put(L"plan", std::to_wstring(r.plan));
    put(L"written", std::to_wstring(r.written_unix));
}

void native_resume_clear(const std::wstring& ini_path) {
    WritePrivateProfileStringW(kSec, nullptr, nullptr, ini_path.c_str());  // the whole section
}

bool native_resume_fresh(const NativeResume& r) {
    const uint64_t now = unix_now();
    return r.mode != NativeMode::Solo && r.written_unix && now >= r.written_unix && now - r.written_unix < kResumeFreshS;
}

}  // namespace cdmp::net
