# Crimson Desert Seamless Co-op+ diagnostics. Started by "Diagnose Co-op+.cmd".
# Writes CoopPlus-report-<date>.txt to the Desktop: game and mod versions, which mod files are
# installed (with hashes), and the end of cdmp.log. Send that file when something goes wrong.
# Reads only; changes nothing. The report contains no personal data beyond your Steam display
# name if it appears in the log.

param(
    [string]$GameDir = '',
    [int]$LogLines = 300,
    [string]$OutDir = '',      # default: the Desktop
    [switch]$NoPause
)
$ErrorActionPreference = 'Continue'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $here 'coopplus-common.ps1')

$out = New-Object System.Collections.Generic.List[string]
function Add([string]$line) { $out.Add($line) }

Add "Crimson Desert Seamless Co-op+ diagnostics, $(Get-Date -Format s)"
Add "Windows: $([Environment]::OSVersion.VersionString)"
$pkg = Get-ChildItem $here -Filter 'README.txt' -ErrorAction SilentlyContinue | Select-Object -First 1
if ($pkg) { Add "Package: $((Get-Content $pkg.FullName -TotalCount 1).Trim())" }

if (-not $GameDir) { $GameDir = Find-CrimsonDesertBin64 }
Add "Game folder: $GameDir"
if ($GameDir -and (Test-Path (Join-Path $GameDir 'CrimsonDesert.exe'))) {
    Add "Game exe version: $((Get-Item (Join-Path $GameDir 'CrimsonDesert.exe')).VersionInfo.FileVersion)"
    Add "Game running now: $([bool](Get-Process -Name 'CrimsonDesert' -ErrorAction SilentlyContinue))"
    Add ''
    Add '--- mod files in bin64'
    foreach ($f in 'winmm.dll', 'cdmp.asi', 'cdmp.ini', 'coopplus-installed.txt', 'dinput8.dll', 'winhttp.dll', 'version.dll') {
        $p = Join-Path $GameDir $f
        if (Test-Path $p) {
            $i = Get-Item $p
            Add ("{0,-24} {1,10} bytes  {2}  sha256 {3}" -f $f, $i.Length, $i.LastWriteTime.ToString('s'),
                 (Get-FileHash $p -Algorithm SHA256).Hash.Substring(0, 16).ToLower())
        }
    }
    $asis = Get-ChildItem $GameDir -Filter '*.asi' -ErrorAction SilentlyContinue | Where-Object Name -ne 'cdmp.asi'
    if ($asis) { Add ("Other ASI mods: " + (($asis | ForEach-Object Name) -join ', ')) }
    $ini = Join-Path $GameDir 'cdmp.ini'
    if (Test-Path $ini) {
        Add ''
        Add '--- cdmp.ini (settings only)'
        $bytes = [IO.File]::ReadAllBytes($ini)
        $text = if ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE) {
            [Text.Encoding]::Unicode.GetString($bytes)
        } else { [Text.Encoding]::UTF8.GetString($bytes) }
        foreach ($l in ($text -split "`r?`n")) {
            $t = $l.Trim()
            if ($t -and -not $t.StartsWith(';')) { Add $t }
        }
    }
    $log = Join-Path $GameDir 'cdmp.log'
    Add ''
    if (Test-Path $log) {
        Add "--- cdmp.log (last $LogLines lines; written $((Get-Item $log).LastWriteTime.ToString('s')))"
        Get-Content $log -Tail $LogLines | ForEach-Object { Add $_ }
    } else {
        Add '--- no cdmp.log: the mod has not run (is winmm.dll there? did the game start through Steam?)'
    }
} else {
    Add 'Crimson Desert not found through Steam.'
}

$desktop = if ($OutDir) { $OutDir } else { [Environment]::GetFolderPath('Desktop') }
$file = Join-Path $desktop ("CoopPlus-report-{0}.txt" -f (Get-Date -Format 'yyyy-MM-dd_HH-mm-ss'))
$out | Set-Content -Path $file -Encoding UTF8
Write-Host "Report written: $file" -ForegroundColor Green
Write-Host 'Send this file to whoever helps you (it reads the game folder and the mod log only).'
if (-not $NoPause) { Write-Host ''; Read-Host 'Press Enter to close' | Out-Null }
