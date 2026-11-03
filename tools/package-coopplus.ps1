# Builds the zip players install: dist\CrimsonDesert-SeamlessCoop-<version>.zip
#
#   .\tools\package-coopplus.ps1               # build, test, package
#   .\tools\package-coopplus.ps1 -SkipBuild    # package the last build (it must match HEAD)
#
# Contents: mod\winmm.dll (Ultimate ASI Loader v9.7.4, hash-checked), mod\cdmp.asi, mod\cdmp.ini
# (one profile for everyone: role=auto), the installer and uninstaller, README, credits, licences,
# source.zip (the GPL source of exactly this build) and SHA256.txt. The debug symbols go to a
# separate zip to keep, not to publish.
#
# Both players must install the same zip: the mod compares a build id derived from the commit,
# so this refuses to package uncommitted changes to the mod's source (-AllowDirty overrides,
# for local tests only).

param(
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo')]
    [string]$Config = 'RelWithDebInfo',
    [switch]$SkipBuild,
    [switch]$AllowDirty
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$up = Join-Path $root 'upstream\cd-coop-3446'
$bin = Join-Path $up "build\bin\$Config"
$kit = Join-Path $root 'kit\coopplus'

function Invoke-Git { & git.exe -C $root @args; if ($LASTEXITCODE -ne 0) { throw "git $args failed" } }

$dirty = Invoke-Git status --porcelain --untracked-files=no -- upstream/cd-coop-3446/src upstream/cd-coop-3446/CMakeLists.txt
if ($dirty -and -not $AllowDirty) {
    throw "Uncommitted changes in the mod's source; commit first so both players get the same build id.`n$($dirty -join "`n")"
}
$sha = (Invoke-Git rev-parse --short=8 HEAD).Trim()
$cmake = Get-Content (Join-Path $root 'CMakeLists.txt') -Raw
if ($cmake -notmatch 'project\(\w+ VERSION ([0-9.]+)') { throw 'No VERSION in CMakeLists.txt' }
$version = "$($Matches[1])-alpha+$sha"

if (-not $SkipBuild) {
    # build.ps1 configures (refreshing the build id), builds and runs the unit tests.
    & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $up 'scripts\build.ps1') -Config $Config
    if ($LASTEXITCODE -ne 0) { throw 'Build or unit tests failed' }
}
foreach ($f in 'cdmp.asi', 'cdmp.pdb') {
    if (-not (Test-Path (Join-Path $bin $f))) { throw "Build first: $f not found in $bin" }
}
# The binary must come from HEAD, or the two players' build ids will not match what the
# package name says.
$idHeader = Get-Content (Join-Path $up 'build\generated\build_id.h') -Raw
if ($idHeader -notmatch 'CDMP_BUILD_ID\s+0x([0-9a-fA-F]+)') { throw 'No CDMP_BUILD_ID in build_id.h' }
if ($Matches[1].ToLower() -ne $sha.ToLower() -and -not $AllowDirty) {
    throw "cdmp.asi was built from $($Matches[1]), HEAD is $sha. Build again (without -SkipBuild)."
}

$loader = & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'fetch-asi-loader.ps1') | Select-Object -Last 1
if (-not (Test-Path $loader)) { throw 'Ultimate ASI Loader not available' }

$name = "CrimsonDesert-SeamlessCoop-$version"
$out = Join-Path $root 'dist'
$stage = Join-Path $out $name
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force (Join-Path $stage 'mod') | Out-Null

Copy-Item $loader (Join-Path $stage 'mod\winmm.dll')
Copy-Item (Join-Path $bin 'cdmp.asi') (Join-Path $stage 'mod\cdmp.asi')
# Co-op+ settings the mod reads from bin64\cdcoop\config (party scaling curves).
New-Item -ItemType Directory -Force (Join-Path $stage 'mod\cdcoop\config') | Out-Null
Copy-Item (Join-Path $root 'config\scaling.toml') (Join-Path $stage 'mod\cdcoop\config\scaling.toml')

# One profile for both players, from upstream's tested co-op profile: role=auto (host your
# world, become a guest on a Steam join), no developer cross on the player. UTF-16 LE with BOM
# and CRLF, which is what the mod's wide INI reader expects.
$bytes = [IO.File]::ReadAllBytes((Join-Path $up 'kit\public\cdmp.ini.anfitrion'))
$text = [Text.Encoding]::Unicode.GetString($bytes).TrimStart([char]0xFEFF)
$lines = $text -split "`r?`n"
if (-not ($lines -contains 'role=host')) { throw 'The upstream host profile changed: no role=host line' }
$lines = $lines | ForEach-Object {
    switch -Regex ($_) {
        '^; Crimson Desert Multiplayer - co-op' { "; Crimson Desert Seamless Co-op+ $version - one profile for both players"; break }
        '^role=host$' {
            'role=auto'
            '; Who may join you: friends = any Steam friend playing ("Join Game"); invite = only friends you invite (Ctrl+F1)'
            'join=friends'
            break
        }
        '^self_marker=1$' { 'self_marker=0'; break }
        '^guest_loot=1$' {
            'guest_loot=1'
            '; Co-op+: who loots a kill you both took part in. per_player = each of you takes your own drop (default);'
            ';   host_only = only the host; first_come = whoever loots first takes it, the other body is empty.'
            'loot_mode=per_player'
            '; Co-op+: 0 = the guest never finishes one of the host''s enemies (it stops at 1 HP for the host).'
            'guest_finishes=1'
            '; Co-op+: 1 = you can revive your friend where they fell (hold Shift+F7 by them, 30 s).'
            'teammate_revive=1'
            break
        }
        default { $_ }
    }
}
$ini = ($lines -join "`r`n").TrimEnd() + "`r`n"
[IO.File]::WriteAllText((Join-Path $stage 'mod\cdmp.ini'), $ini, (New-Object Text.UnicodeEncoding($false, $true)))

foreach ($f in 'install.ps1', 'uninstall.ps1', 'diagnose.ps1', 'vanilla.ps1', 'coopplus-common.ps1', 'Install Co-op+.cmd', 'Uninstall Co-op+.cmd', 'Diagnose Co-op+.cmd', 'Vanilla mode on-off.cmd') {
    Copy-Item (Join-Path $kit $f) $stage
}
(Get-Content (Join-Path $kit 'README.txt') -Raw).Replace('@VERSION@', $version) |
    Set-Content -Encoding ascii (Join-Path $stage 'README.txt')
Copy-Item (Join-Path $root 'CREDITS.md') $stage
Copy-Item (Join-Path $root 'CHANGELOG.md') $stage
Copy-Item (Join-Path $root 'LICENSE') (Join-Path $stage 'LICENSE.txt')
Copy-Item (Join-Path $up 'kit\public\THIRD-PARTY-NOTICES.txt') $stage
Invoke-Git archive --format=zip -o (Join-Path $stage 'source.zip') HEAD

Get-ChildItem $stage -File -Recurse | Where-Object Name -ne 'SHA256.txt' | ForEach-Object {
    '{0}  {1}' -f (Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLower(), $_.FullName.Substring($stage.Length + 1)
} | Set-Content -Encoding ascii (Join-Path $stage 'SHA256.txt')

$zip = Join-Path $out "$name.zip"
if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip
$symbols = Join-Path $out "$name-symbols-KEEP-PRIVATE.zip"
if (Test-Path $symbols) { Remove-Item $symbols }
Compress-Archive -Path (Join-Path $bin 'cdmp.pdb'), (Join-Path $bin 'cdmp.asi') -DestinationPath $symbols

Write-Host "Package: $zip ($([math]::Round((Get-Item $zip).Length / 1KB)) KB)"
Write-Host "Symbols (keep, do not publish): $symbols"
