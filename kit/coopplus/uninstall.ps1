# Crimson Desert Seamless Co-op+ uninstaller. Started by "Uninstall Co-op+.cmd".
# Removes exactly the files the installer listed in coopplus-installed.txt and puts back
# anything it kept as *.before-coopplus. Save backups in Documents are left alone.

param(
    [string]$GameDir = '',
    [switch]$NoPause
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'coopplus-common.ps1')

function Done([int]$code) {
    if (-not $NoPause) { Write-Host ''; Read-Host 'Press Enter to close' | Out-Null }
    exit $code
}

Write-Host 'Crimson Desert Seamless Co-op+ - uninstall' -ForegroundColor Cyan
if (-not $GameDir) { $GameDir = Find-CrimsonDesertBin64 }
if (-not $GameDir -or -not (Test-Path $GameDir)) {
    Write-Host 'Crimson Desert was not found through Steam. Use -GameDir "<...>\Crimson Desert\bin64".' -ForegroundColor Red
    Done 1
}
if (Get-Process -Name 'CrimsonDesert' -ErrorAction SilentlyContinue) {
    Write-Host 'Crimson Desert is running. Close the game first.' -ForegroundColor Red
    Done 1
}
$list = Join-Path $GameDir 'coopplus-installed.txt'
if (-not (Test-Path $list)) {
    Write-Host "Co-op+ is not installed in $GameDir (no coopplus-installed.txt). Nothing removed." -ForegroundColor Yellow
    Done 0
}
foreach ($line in Get-Content $list) {
    $name = $line.Trim()
    if (-not $name -or $name.StartsWith('#')) { continue }
    # Only names the installer writes: plain file names, or files under cdcoop\config\.
    if ($name -notmatch '^[A-Za-z0-9_.+ -]+$' -and $name -notmatch '^cdcoop\\config\\[A-Za-z0-9_.-]+$') { continue }
    if ($name -match '\.\.') { continue }
    $path = Join-Path $GameDir $name
    if ((Test-Path $path) -and $name -ne 'coopplus-installed.txt') {
        Remove-Item $path -Force
        Write-Host "Removed $name"
    }
    $kept = "$path.before-coopplus"
    if (Test-Path $kept) {
        Move-Item $kept $path -Force
        Write-Host "Restored your earlier $name"
    }
}
# Logs the mod wrote next to itself.
foreach ($log in @(Get-ChildItem $GameDir -Filter 'cdmp*.log' -ErrorAction SilentlyContinue) +
                 @(Get-ChildItem $GameDir -Filter 'coopplus*.log' -ErrorAction SilentlyContinue)) {
    Remove-Item $log.FullName -Force
}
Remove-Item $list -Force
# Our settings folder, if it is now empty.
foreach ($d in (Join-Path $GameDir 'cdcoop\config'), (Join-Path $GameDir 'cdcoop')) {
    if ((Test-Path $d) -and -not (Get-ChildItem $d -Force)) { Remove-Item $d -Force }
}
# Vanilla mode (vanilla.ps1) parks the mod under this name.
$parked = Join-Path $GameDir 'cdmp.asi.off'
if (Test-Path $parked) { Remove-Item $parked -Force; Write-Host 'Removed cdmp.asi.off (vanilla mode)' }
$parkedLayer = Join-Path $GameDir 'coopplus.asi.off'
if (Test-Path $parkedLayer) { Remove-Item $parkedLayer -Force; Write-Host 'Removed coopplus.asi.off (vanilla mode)' }
Write-Host ''
Write-Host 'Co-op+ removed. The game runs as before. Your save backups stay in Documents\CrimsonDesert-CoopPlus-SaveBackups.' -ForegroundColor Green
Done 0
