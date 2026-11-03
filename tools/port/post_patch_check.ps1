# Post-patch check (roadmap T38, docs/post-patch-playbook.md): after a game update, what broke?
#  1. the offline verifier (upstream/cd-coop-3446/scripts/verify-port.ps1) against the installed exe;
#  2. every hook / offset problem the mod logged in its last run (cdmp.log in the game's bin64).
#   powershell -ExecutionPolicy Bypass -File tools\port\post_patch_check.ps1 [-GameBin "D:\...\bin64"]
param([string]$GameBin = '')
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
if (-not $GameBin) {
    $steam = (Get-ItemProperty 'HKCU:\Software\Valve\Steam' -ErrorAction SilentlyContinue).SteamPath
    foreach ($lib in @("$steam\steamapps\common", 'D:\SteamLibrary\steamapps\common', 'C:\Program Files (x86)\Steam\steamapps\common')) {
        if (Test-Path "$lib\Crimson Desert\bin64\CrimsonDesert.exe") { $GameBin = "$lib\Crimson Desert\bin64"; break }
    }
}
if (-not (Test-Path "$GameBin\CrimsonDesert.exe")) { throw "CrimsonDesert.exe not found; pass -GameBin" }
$exe = "$GameBin\CrimsonDesert.exe"
"Game exe: $exe  version $((Get-Item $exe).VersionInfo.FileVersion)"

"`n== Offline verifier"
Push-Location "$root\upstream\cd-coop-3446"
try { & powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify-port.ps1 -Exe $exe 2>&1 | Select-Object -Last 25 }
finally { Pop-Location }

"`n== The mod's last log ($GameBin\cdmp.log)"
if (Test-Path "$GameBin\cdmp.log") {
    $bad = Select-String -Path "$GameBin\cdmp.log" -Pattern 'not recognised', 'hook failed', 'FAILED to install', 'crash guards',
        'Crash log', 'is off: this mod was made', 'not installed', 'did not install', 'NOT recognised'
    if ($bad) { $bad | ForEach-Object { $_.Line } } else { 'no hook or offset problems logged' }
} else { 'no cdmp.log yet: start the game once with Co-op+ installed' }
