#pragma once
// M0 Recon probe: scans the game for the signatures in config/probe.toml, follows the pointer chains and logs the
// values (player position, health, world time...) every few seconds. Read-only: it never writes game memory.
#include <filesystem>

namespace cdcoop::game {
// Starts the probe on its own thread. Returns false (and logs why) if the config is missing or invalid.
bool start_probe(const std::filesystem::path& probe_toml);
}  // namespace cdcoop::game
