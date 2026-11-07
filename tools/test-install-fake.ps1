# T27: clean-machine walkthrough of the release zips on a FAKE game folder (never the real one).
#
#   .\tools\test-install-fake.ps1 [-Zip dist\...zip]   (default: the newest player zip in dist)
#
# A temp folder gets "Crimson Desert\bin64" with a stand-in CrimsonDesert.exe and an earlier winmm.dll of
# "another mod", plus a stand-in save folder. Then, as a player would: unzip, install, play-edit the settings,
# install again (update), vanilla on and off, uninstall; and the mod-manager zip unpacked by hand. After the
# uninstall the folder must be byte-for-byte what it was before. Prints PASS/FAIL per step; exit 1 on any FAIL.
param([string]$Zip = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (-not $Zip) {
    $Zip = (Get-ChildItem (Join-Path $root 'dist') -Filter 'CrimsonDesert-SeamlessCoop-*.zip' |
        Where-Object { $_.Name -notmatch 'symbols|mod-manager' } | Sort-Object LastWriteTime | Select-Object -Last 1).FullName
}
$managerZip = $Zip -replace '\.zip$', '-mod-manager.zip'
$t = Join-Path ([IO.Path]::GetTempPath()) ("coopplus-fake-" + (Get-Date -Format 'HHmmss'))
$game = Join-Path $t 'Crimson Desert'; $bin = Join-Path $game 'bin64'
$saves = Join-Path $t 'save'; $backups = Join-Path $t 'backups'; $kit = Join-Path $t 'download'
New-Item -ItemType Directory -Force $bin, $saves | Out-Null
Copy-Item "$env:WINDIR\System32\where.exe" (Join-Path $bin 'CrimsonDesert.exe')  # any exe: the version note is expected
Set-Content (Join-Path $bin 'winmm.dll') 'an earlier loader from another mod'
Set-Content (Join-Path $saves 'slot0.save') 'stand-in save'
$fails = 0
function Check([string]$what, [bool]$ok) {
    if ($ok) { Write-Host "PASS  $what" -ForegroundColor Green } else { Write-Host "FAIL  $what" -ForegroundColor Red; $script:fails++ }
}
function Snapshot([string]$dir) {
    Get-ChildItem $dir -Recurse -File | ForEach-Object { '{0} {1}' -f $_.FullName.Substring($dir.Length), (Get-FileHash $_.FullName).Hash } | Sort-Object
}
$before = Snapshot $bin
Write-Host "Fake game: $game"
Write-Host "Zip: $Zip"

Expand-Archive $Zip $kit
$present = @('README.txt', 'BUG-REPORT.txt', 'OTHER-MODS.md', 'CREDITS.md', 'LICENSE.txt', 'source.zip', 'SHA256.txt', 'Install Co-op+.cmd', 'Uninstall Co-op+.cmd') |
    ForEach-Object { Test-Path (Join-Path $kit $_) }
Check 'zip has README, BUG-REPORT, OTHER-MODS, CREDITS, LICENSE, source.zip, SHA256' ($present -notcontains $false)
$bad = Get-Content (Join-Path $kit 'SHA256.txt') | Where-Object { $_ } | Where-Object {
    $h, $f = $_ -split '  ', 2; (Get-FileHash (Join-Path $kit $f)).Hash.ToLower() -ne $h }
Check 'SHA256.txt matches every file' (-not $bad)

$o = & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $kit 'install.ps1') -GameDir $bin -SaveDir $saves -BackupRoot $backups -NoPause 2>&1 | Out-String
Check 'install: says Installed' ($o -match 'Installed\.')
Check 'install: mod files in bin64' ((@('winmm.dll', 'cdmp.asi', 'cdmp.ini', 'cdcoop\config\scaling.toml') | ForEach-Object { Test-Path (Join-Path $bin $_) }) -notcontains $false)
Check 'install: the other mod''s winmm.dll kept' ((Get-Content (Join-Path $bin 'winmm.dll.before-coopplus') -Raw) -match 'another mod')
Check 'install: saves backed up' ((Get-ChildItem $backups -Recurse -File | Measure-Object).Count -eq 1)
Check 'install: version note for another game version' ($o -match 'this mod was built for')

# A player edits the settings; an update must keep them.
$ini = Join-Path $bin 'cdmp.ini'
$text = [IO.File]::ReadAllText($ini, [Text.Encoding]::Unicode) -replace 'join=friends', 'join=invite'
# ... and it came from an older zip, which shipped the guest's stagger off under the old comment (the update turns it on).
$text = [regex]::Replace($text, '(?m)^; The stagger reaction on those blows.*\r?\n; 0 = damage only.*\r?\nguest_hit_stagger=1',
    "; The stagger reaction on those blows. 0 is safer: it injects a hit reaction`r`n; into the other player's enemy, which its AI may not expect.`r`nguest_hit_stagger=0")
if ($text -notmatch 'guest_hit_stagger=0') { Check 'update: an older ini made for the stagger test' $false }
[IO.File]::WriteAllText($ini, $text, (New-Object Text.UnicodeEncoding($false, $true)))
$o = & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $kit 'install.ps1') -GameDir $bin -SaveDir $saves -BackupRoot $backups -NoPause 2>&1 | Out-String
Check 'update: player settings kept, new defaults as cdmp.ini.new' (([IO.File]::ReadAllText($ini, [Text.Encoding]::Unicode) -match 'join=invite') -and (Test-Path "$ini.new"))
Check 'update: an older zip''s guest_hit_stagger=0 turned on, once' (([IO.File]::ReadAllText($ini, [Text.Encoding]::Unicode) -match '(?m)^guest_hit_stagger=1') -and ($o -match 'Turned on: the guest'))

& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $kit 'vanilla.ps1') -GameDir $bin -Mode on -NoPause | Out-Null
Check 'vanilla on: cdmp.asi parked' ((Test-Path (Join-Path $bin 'cdmp.asi.off')) -and -not (Test-Path (Join-Path $bin 'cdmp.asi')))
& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $kit 'vanilla.ps1') -GameDir $bin -Mode off -NoPause | Out-Null
Check 'vanilla off: cdmp.asi back' (Test-Path (Join-Path $bin 'cdmp.asi'))

Set-Content (Join-Path $bin 'cdmp.log') 'a log the mod wrote'
& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $kit 'uninstall.ps1') -GameDir $bin -NoPause | Out-Null
$after = Snapshot $bin
Check 'uninstall: bin64 exactly as before the install' (-not (Compare-Object $before $after))
Check 'uninstall: save backups left alone' ((Get-ChildItem $backups -Recurse -File | Measure-Object).Count -ge 1)

if (Test-Path $managerZip) {
    Expand-Archive $managerZip $game -Force
    Check 'mod-manager zip: cdmp.asi and winmm.dll land next to CrimsonDesert.exe' (
        (Test-Path (Join-Path $bin 'cdmp.asi')) -and (Test-Path (Join-Path $bin 'cdcoop\config\scaling.toml')) -and
        (Get-FileHash (Join-Path $bin 'cdmp.asi')).Hash -eq (Get-FileHash (Join-Path $kit 'mod\cdmp.asi')).Hash)
} else {
    Check "mod-manager zip present ($managerZip)" $false
}
Remove-Item -Recurse -Force $t
Write-Host ''
if ($fails) { Write-Host "$fails step(s) FAILED" -ForegroundColor Red; exit 1 }
Write-Host 'All steps passed' -ForegroundColor Green
