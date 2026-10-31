# Crimson Desert Seamless Co-op+ installer.
# Started by "Install Co-op+.cmd". Safe to run again (it updates the files).
#
# What it does, in order, and it stops at the first problem:
#   1. finds the game through Steam (or uses -GameDir);
#   2. refuses while the game is running;
#   3. copies your saves to Documents\CrimsonDesert-CoopPlus-SaveBackups\<date-time>;
#   4. keeps any winmm.dll / cdmp.asi / cdmp.ini already in the game folder as *.before-coopplus;
#   5. copies the mod files into the game's bin64 folder and lists them in coopplus-installed.txt,
#      which "Uninstall Co-op+.cmd" uses to remove exactly those files.
# Nothing else on the PC is changed.

param(
    [string]$GameDir = '',     # the game's bin64 folder (found through Steam when empty)
    [string]$SaveDir = '',     # the save folder (default: %LOCALAPPDATA%\Pearl Abyss\CD\save)
    [string]$BackupRoot = '',  # where save backups go (default: Documents\CrimsonDesert-CoopPlus-SaveBackups)
    [switch]$NoPause
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$payload = Join-Path $here 'mod'
. (Join-Path $here 'coopplus-common.ps1')
$expectedExeVersion = '1.0.0.3460'
$files = @('winmm.dll', 'cdmp.asi', 'cdmp.ini', 'cdcoop\config\scaling.toml')
# Files players edit: an update keeps theirs and puts the new defaults next to them.
$settingsFiles = @('cdmp.ini', 'cdcoop\config\scaling.toml')

function Done([int]$code) {
    if (-not $NoPause) { Write-Host ''; Read-Host 'Press Enter to close' | Out-Null }
    exit $code
}
function Fail([string]$msg) {
    Write-Host ''
    Write-Host "NOT INSTALLED: $msg" -ForegroundColor Red
    Done 1
}

Write-Host 'Crimson Desert Seamless Co-op+ - install' -ForegroundColor Cyan
Write-Host ''

foreach ($f in $files) {
    if (-not (Test-Path (Join-Path $payload $f))) { Fail "the package is incomplete ($f missing). Download it again." }
}

if (-not $GameDir) { $GameDir = Find-CrimsonDesertBin64 }
if (-not $GameDir -or -not (Test-Path (Join-Path $GameDir 'CrimsonDesert.exe'))) {
    Fail "Crimson Desert was not found through Steam. Run this again from PowerShell with -GameDir `"<your Steam library>\steamapps\common\Crimson Desert\bin64`"."
}
Write-Host "Game folder: $GameDir"

if (Get-Process -Name 'CrimsonDesert' -ErrorAction SilentlyContinue) {
    Fail 'Crimson Desert is running. Close the game first, then run the installer again.'
}

$exeVersion = (Get-Item (Join-Path $GameDir 'CrimsonDesert.exe')).VersionInfo.FileVersion
if ($exeVersion -ne $expectedExeVersion) {
    Write-Host "Note: your game is version $exeVersion; this mod was built for $expectedExeVersion." -ForegroundColor Yellow
    Write-Host '      On another version the mod switches itself off and the game runs normally.' -ForegroundColor Yellow
}

# Saves first, always.
if (-not $SaveDir) { $SaveDir = Join-Path $env:LOCALAPPDATA 'Pearl Abyss\CD\save' }
if (-not $BackupRoot) { $BackupRoot = Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'CrimsonDesert-CoopPlus-SaveBackups' }
if (Test-Path $SaveDir) {
    $dest = Join-Path $BackupRoot (Get-Date -Format 'yyyy-MM-dd_HH-mm-ss')
    New-Item -ItemType Directory -Force $dest | Out-Null
    Copy-Item -Path (Join-Path $SaveDir '*') -Destination $dest -Recurse -Force
    $count = (Get-ChildItem $dest -Recurse -File | Measure-Object).Count
    if ($count -eq 0 -and (Get-ChildItem $SaveDir -Recurse -File | Measure-Object).Count -gt 0) {
        Fail "the save backup to $dest came out empty, so nothing was installed."
    }
    Write-Host "Saves backed up: $count file(s) -> $dest" -ForegroundColor Green
} else {
    Write-Host "No save folder at $SaveDir (new install?); nothing to back up." -ForegroundColor Yellow
}

# Keep whatever was there before, once (never overwrite an earlier keep). An update
# keeps the earlier list too, so Uninstall still removes everything we ever put there.
$listPath = Join-Path $GameDir 'coopplus-installed.txt'
$installed = @()
if (Test-Path $listPath) {
    $installed += Get-Content $listPath | Where-Object { $_ -and -not $_.StartsWith('#') -and $_ -ne 'coopplus-installed.txt' }
}
# Vanilla mode (vanilla.ps1) parks the mod as cdmp.asi.off; installing turns it back on.
$parked = Join-Path $GameDir 'cdmp.asi.off'
if (Test-Path $parked) {
    Remove-Item $parked -Force
    Write-Host 'Vanilla mode was on; this install turns Co-op+ back on.'
}
foreach ($f in $files) {
    $target = Join-Path $GameDir $f
    $source = Join-Path $payload $f
    if (Test-Path $target) {
        $same = (Get-FileHash $target).Hash -eq (Get-FileHash $source).Hash
        $kept = "$target.before-coopplus"
        $ours = Test-Path (Join-Path $GameDir 'coopplus-installed.txt')
        if (-not $same -and -not $ours -and -not (Test-Path $kept)) {
            Copy-Item $target $kept
            Write-Host "Kept your existing $f as $f.before-coopplus"
        }
        if ($settingsFiles -contains $f -and $ours -and -not $same) {
            # An update keeps the player's own settings; the new defaults go next to them.
            Copy-Item $source "$target.new" -Force
            Write-Host "Kept your $f settings (new defaults saved as $f.new)"
            $installed += $f, "$f.new"
            continue
        }
    }
    New-Item -ItemType Directory -Force (Split-Path -Parent $target) | Out-Null
    Copy-Item $source $target -Force
    $installed += $f
}
$installed = @($installed | Select-Object -Unique) + 'coopplus-installed.txt'
Set-Content -Path $listPath -Encoding ascii -Value (
    @("# Crimson Desert Seamless Co-op+ installed $(Get-Date -Format s). Files below are removed by Uninstall.") + $installed)

Write-Host ''
Write-Host 'Installed. Start Crimson Desert from Steam as usual.' -ForegroundColor Green
Write-Host 'In game: F8 shows the co-op panel, Ctrl+F1 invites a Steam friend.'
Write-Host 'Your friend accepts the invite (or uses "Join Game" on you in Steam) and lands in your world.'
Done 0
