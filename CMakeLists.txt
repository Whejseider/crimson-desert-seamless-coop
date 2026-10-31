# Crimson Desert Seamless Co-op+ (working name "cdcoop")
# Builds:
#   cdcoop_core   - pure, platform-independent logic (sim/, net/protocol, game/sigscan, config). Unit-tested on any OS.
#   cdcoop        - the Windows x64 DLL (shipped as CrimsonDesertCoop.asi for an ASI loader). Windows / MinGW / MSVC only.
#   cdcoop_tests  - unit tests for cdcoop_core (run with ctest).
cmake_minimum_required(VERSION 3.20)
project(CrimsonDesertSeamlessCoop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(CDCOOP_BUILD_TESTS "Build unit tests" ON)

if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8 /EHsc)
  # Static CRT: friends don't need the Visual C++ redistributable.
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

configure_file(src/core/version.hpp.in ${CMAKE_BINARY_DIR}/generated/cdcoop_version.hpp @ONLY)

# ---------------------------------------------------------------- pure core
add_library(cdcoop_core STATIC
  src/sim/scaling.cpp
  src/sim/loot.cpp
  src/sim/progression.cpp
  src/sim/config.cpp
  src/net/protocol.cpp
  src/game/sigscan.cpp
  src/game/probe_spec.cpp
)
target_include_directories(cdcoop_core PUBLIC src ${CMAKE_BINARY_DIR}/generated)
target_include_directories(cdcoop_core SYSTEM PUBLIC third_party/tomlplusplus)
target_compile_definitions(cdcoop_core PUBLIC TOML_EXCEPTIONS=0)

# ---------------------------------------------------------------- the DLL (Windows only)
if(WIN32)
  add_library(cdcoop SHARED
    src/core/dllmain.cpp
    src/core/log.cpp
    src/core/paths.cpp
    src/core/safety.cpp
    src/game/probe.cpp
    src/compat/environment.cpp
    src/net/steam_runtime.cpp
  )
  target_link_libraries(cdcoop PRIVATE cdcoop_core psapi version)
  target_compile_definitions(cdcoop PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
  set_target_properties(cdcoop PROPERTIES OUTPUT_NAME "CrimsonDesertCoop" PREFIX "" SUFFIX ".asi")
  if(MINGW)
    target_link_options(cdcoop PRIVATE -static -static-libgcc -static-libstdc++)
  endif()
  install(TARGETS cdcoop RUNTIME DESTINATION . LIBRARY DESTINATION .)
  install(DIRECTORY config/ DESTINATION cdcoop/config)
endif()

# ---------------------------------------------------------------- tests
if(CDCOOP_BUILD_TESTS)
  enable_testing()
  add_executable(cdcoop_tests
    tests/test_main.cpp
    tests/test_scaling.cpp
    tests/test_loot.cpp
    tests/test_progression.cpp
    tests/test_config.cpp
    tests/test_protocol.cpp
    tests/test_sigscan.cpp
    tests/test_probe_spec.cpp
    tests/test_interpolation.cpp
  )
  target_link_libraries(cdcoop_tests PRIVATE cdcoop_core)
  target_compile_definitions(cdcoop_tests PRIVATE CDCOOP_CONFIG_DIR="${CMAKE_SOURCE_DIR}/config")
  if(MINGW)
    target_link_options(cdcoop_tests PRIVATE -static)
  endif()
  add_test(NAME cdcoop_tests COMMAND cdcoop_tests)

  # Upstream (cd-coop-3446) unit tests: its portable src/common logic, built here on any OS.
  # (Its full mod, cdmp.asi, is MSVC-only and is built by upstream/cd-coop-3446/CMakeLists.txt on Windows.)
  set(UP ${CMAKE_SOURCE_DIR}/upstream/cd-coop-3446)
  add_executable(upstream_tests
    ${UP}/tests/tests.cpp
    ${UP}/src/common/pattern.cpp
    ${UP}/src/common/protocol.cpp
    ${UP}/src/common/client_session.cpp
    ${UP}/src/common/reliable.cpp
    ${UP}/src/common/enemy_load.cpp
  )
  target_include_directories(upstream_tests PRIVATE ${UP}/src)
  target_compile_definitions(upstream_tests PRIVATE CDMP_VERSION="upstream")
  target_compile_options(upstream_tests PRIVATE $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-w>)
  if(WIN32)
    target_link_libraries(upstream_tests PRIVATE ws2_32)
  endif()
  if(MINGW)
    target_link_options(upstream_tests PRIVATE -static)
  endif()
  add_test(NAME upstream_tests COMMAND upstream_tests)
endif()
