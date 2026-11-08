# B01: an existing Co-op+ install (the normal zip, the player's settings in cdmp.ini) updated to the SHARED-WORLD
# zip, on a FAKE game folder (never the real one). The player's settings must move to coopplus.ini, cdmp.ini must
# become Co-op+'s Solo launch file, and an uninstall must still leave bin64 exactly as it was.
#   .\tools\test-install-upgrade.ps1 -Normal dist\...<sha>.zip -Shared dist\...<sha>-shared-world.zip
param([Parameter(Mandatory = $true)][string]$Normal, [Parameter(Mandatory = $true)][string]$Shared)
$ErrorActionPreference = 'Stop'
$t = Join-Path ([IO.Path]::GetTempPath()) ("coopplus-upgrade-" + (Get-Date -Format 'HHmmss'))
$game = Join-Path $t 'Crimson Desert'; $bin = Join-Path $game 'bin64'
$saves = Join-Path $t 'save'; $backups = Join-Path $t 'backups'
New-Item -ItemType Directory -Force $bin, $saves | Out-Null
Copy-Item "$env:WINDIR\System32\where.exe" (Join-Path $bin 'CrimsonDesert.exe')
Set-Content (Join-Path $saves 'slot0.save') 'stand-in save'
$fails = 0
function Check([string]$what, [bool]$ok) {
    if ($ok) { Write-Host "PASS  $what" -ForegroundColor Green } else { Write-Host "FAIL  $what" -ForegroundColor Red; $script:fails++ }
}
function Snapshot([string]$dir) {
    Get-ChildItem $dir -Recurse -File | ForEach-Object { '{0} {1}' -f $_.FullName.Substring($dir.Length), (Get-FileHash $_.FullName).Hash } | Sort-Object
}
function Install([string]$zip, [string]$name) {
    $kit = Join-Path $t $name
    Expand-Archive $zip $kit
    & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $kit 'install.ps1') -GameDir $bin -SaveDir $saves -BackupRoot $backups -NoPause 2>&1 | Out-String
}
$before = Snapshot $bin
$o = Install $Normal 'normal'
Check 'normal install' ($o -match 'Installed\.')
$ini = Join-Path $bin 'cdmp.ini'
$text = [IO.File]::ReadAllText($ini, [Text.Encoding]::Unicode) -replace 'join=friends', 'join=invite'
[IO.File]::WriteAllText($ini, $text, (New-Object Text.UnicodeEncoding($false, $true)))
$o = Install $Shared 'shared'
Check 'shared-world update: says Installed' ($o -match 'Installed\.')
Check 'shared-world update: says the settings moved' ($o -match 'settings moved from cdmp.ini to coopplus.ini')
$co = Join-Path $bin 'coopplus.ini'
Check 'coopplus.ini keeps the player''s join=invite' ((Test-Path $co) -and ([IO.File]::ReadAllText($co, [Text.Encoding]::Unicode) -match 'join=invite'))
Check 'cdmp.ini is the Solo launch file' ([IO.File]::ReadAllText($ini, [Text.Encoding]::Unicode) -match 'coopplus_mode=solo')
Check 'coopplus.asi installed' (Test-Path (Join-Path $bin 'coopplus.asi'))
& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $t 'shared\uninstall.ps1') -GameDir $bin -NoPause | Out-Null
Check 'uninstall: bin64 exactly as before the first install' (-not (Compare-Object $before (Snapshot $bin)))
Remove-Item -Recurse -Force $t
if ($fails) { Write-Host "$fails step(s) FAILED" -ForegroundColor Red; exit 1 }
Write-Host 'All steps passed' -ForegroundColor Green
