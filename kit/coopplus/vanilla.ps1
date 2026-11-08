# One-click vanilla mode (brief 6.7). Started by "Vanilla mode on-off.cmd".
# Turns Co-op+ off by renaming bin64\cdmp.asi to cdmp.asi.off (the ASI loader then skips it),
# and back on by renaming it back. Settings, save backups and any other ASI mods are untouched.

param(
    [string]$GameDir = '',
    [ValidateSet('toggle', 'on', 'off')]
    [string]$Mode = 'toggle',   # on = vanilla on (mod off); off = vanilla off (mod on)
    [switch]$NoPause
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'coopplus-common.ps1')

function Done([int]$code) {
    if (-not $NoPause) { Write-Host ''; Read-Host 'Press Enter to close' | Out-Null }
    exit $code
}

if (-not $GameDir) { $GameDir = Find-CrimsonDesertBin64 }
if (-not $GameDir -or -not (Test-Path $GameDir)) {
    Write-Host 'Crimson Desert was not found through Steam. Use -GameDir "<...>\Crimson Desert\bin64".' -ForegroundColor Red
    Done 1
}
if (Get-Process -Name 'CrimsonDesert' -ErrorAction SilentlyContinue) {
    Write-Host 'Crimson Desert is running. Close the game first.' -ForegroundColor Red
    Done 1
}
$on = Join-Path $GameDir 'cdmp.asi'
$off = Join-Path $GameDir 'cdmp.asi.off'
$isVanilla = (Test-Path $off) -and -not (Test-Path $on)
if (-not (Test-Path $on) -and -not (Test-Path $off)) {
    Write-Host 'Co-op+ is not installed (no cdmp.asi). Use "Install Co-op+.cmd".' -ForegroundColor Yellow
    Done 1
}
$wantVanilla = switch ($Mode) { 'on' { $true } 'off' { $false } default { -not $isVanilla } }
# The shared-world package has the Co-op+ layer too (coopplus.asi): parked and unparked with the base.
$layerOn = Join-Path $GameDir 'coopplus.asi'
$layerOff = Join-Path $GameDir 'coopplus.asi.off'
if ($wantVanilla -and -not $isVanilla) {
    if (Test-Path $off) { Remove-Item $off -Force }
    Rename-Item $on 'cdmp.asi.off'
    if (Test-Path $layerOn) {
        if (Test-Path $layerOff) { Remove-Item $layerOff -Force }
        Rename-Item $layerOn 'coopplus.asi.off'
    }
    Write-Host 'Vanilla mode ON: Co-op+ is off. The game runs as if it were not installed.' -ForegroundColor Green
} elseif (-not $wantVanilla -and $isVanilla) {
    Rename-Item $off 'cdmp.asi'
    if ((Test-Path $layerOff) -and -not (Test-Path $layerOn)) { Rename-Item $layerOff 'coopplus.asi' }
    Write-Host 'Vanilla mode OFF: Co-op+ is on again.' -ForegroundColor Green
} else {
    Write-Host ("Nothing to do: Co-op+ is already {0}." -f ($(if ($isVanilla) { 'off (vanilla)' } else { 'on' })))
}
Done 0
