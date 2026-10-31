// Crimson Desert Seamless Co-op+ entry point. Loaded by an ASI loader as CrimsonDesertCoop.asi.
// DllMain does nothing but start a thread (no work under the loader lock). Everything after that is guarded: any
// failure logs a reason and leaves the game running as vanilla.
#include <windows.h>

#include <algorithm>
#include <cctype>
#include <exception>
#include <filesystem>
#include <string>

#include "cdcoop_version.hpp"
#include "compat/environment.hpp"
#include "core/log.hpp"
#include "core/paths.hpp"
#include "core/safety.hpp"
#include "game/probe.hpp"
#include "sim/config.hpp"

namespace {

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

void init() {
    using namespace cdcoop;
    const auto cfg = sim::load_config(paths::config_dir());
    log::open(paths::logs_dir() / L"cdcoop.log", log::parse_level(cfg.general.log_level));
    log::info(std::string("Crimson Desert Seamless Co-op+ ") + CDCOOP_VERSION_STRING + " loaded from " + paths::utf8(paths::module_dir()));
    for (const auto& e : cfg.errors) log::warn("Config: " + e);

    const std::string exe = lower(paths::utf8(paths::exe_path().filename()));
    if (exe != lower(cfg.general.expected_exe)) {
        safety::disable("not the game process (" + exe + ", expected " + cfg.general.expected_exe + ")");
        return;
    }
    if (!cfg.general.enabled || std::filesystem::exists(paths::data_dir() / L"VANILLA")) {
        safety::disable("vanilla mode is on (general.enabled = false or cdcoop/VANILLA exists)");
        return;
    }

    const auto env = compat::scan();
    compat::log_report(env);

    // M0: read-only recon. Later milestones start net/, sim/ and ui/ from here once their hooks verify.
    game::start_probe(paths::config_dir() / L"probe.toml");
}

DWORD WINAPI init_thread(LPVOID) {
    try {
        init();
    } catch (const std::exception& e) {
        cdcoop::safety::disable(std::string("startup error: ") + e.what());
    } catch (...) {
        cdcoop::safety::disable("unknown startup error");
    }
    return 0;
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        if (HANDLE t = CreateThread(nullptr, 0, init_thread, nullptr, 0, nullptr)) CloseHandle(t);
    }
    return TRUE;
}

// Some ASI loaders call this if present; harmless otherwise.
extern "C" __declspec(dllexport) void InitializeASI() {}
