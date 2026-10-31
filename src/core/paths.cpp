#include "core/paths.hpp"

#include <windows.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace cdcoop::paths {
namespace {
std::filesystem::path module_file(HMODULE m) {
    wchar_t buf[MAX_PATH * 2];
    const DWORD n = GetModuleFileNameW(m, buf, static_cast<DWORD>(std::size(buf)));
    return std::filesystem::path(std::wstring(buf, n));
}
}  // namespace
std::filesystem::path module_dir() { return module_file(reinterpret_cast<HMODULE>(&__ImageBase)).parent_path(); }
std::filesystem::path data_dir() { return module_dir() / L"cdcoop"; }
std::filesystem::path config_dir() { return data_dir() / L"config"; }
std::filesystem::path logs_dir() { return data_dir() / L"logs"; }
std::filesystem::path exe_path() { return module_file(nullptr); }
std::string utf8(const std::filesystem::path& p) {
    const std::wstring& w = p.native();
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<std::size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), s.data(), n, nullptr, nullptr);
    return s;
}
}  // namespace cdcoop::paths
