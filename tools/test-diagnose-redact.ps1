# T37: the Diagnose bundle on a fake game folder with a log full of personal data. Everything personal must be gone
# from the .txt and the zip's cdmp.log; the game version and coordinates must stay. Exit 1 on a leak.
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$t = Join-Path ([IO.Path]::GetTempPath()) ("coopplus-diag-" + (Get-Date -Format 'HHmmss'))
$bin = Join-Path $t 'Crimson Desert\bin64'
New-Item -ItemType Directory -Force $bin | Out-Null
Copy-Item "$env:WINDIR\System32\where.exe" (Join-Path $bin 'CrimsonDesert.exe')
$secrets = @('Kliffhanger_77', 'Oongka Fan', '76561198012345678', '109775241234567890', 'QX7P2KD', "$env:USERNAME", "$env:COMPUTERNAME", '192.168.1.44', 'someone@example.com')
@(
    "[16:00:00.000] INF [T1] Host process: C:\Users\$env:USERNAME\Games\CrimsonDesert.exe (PID 1)",
    "[16:00:00.000] INF [T1] Host version: 1.0.0.3479  image base: 0x140000000",
    "[16:00:00.000] INF [T1] Co-op+: playing as 'Kliffhanger_77' (Steam name)",
    "[16:00:01.000] INF [T1] Room: hosting, code QX7P2KD (lobby 109775241234567890, host SteamID64 76561198012345678)",
    "[16:00:02.000] INF [T1] Co-op: connected to 'Oongka Fan'",
    "[16:00:03.000] INF [T1] Local: our horse at (-10300.12, 535.40, -3915.00) on $env:COMPUTERNAME",
    "[16:00:04.000] INF [T1] Network: udp 192.168.1.44:37015 and 127.0.0.1:37015; mail someone@example.com",
    "[16:00:05.000] WRN [T1] Save guard: the game saved (slot 2) while 'Oongka Fan' is in the session [1 so far]"
) | Set-Content (Join-Path $bin 'cdmp.log') -Encoding UTF8
& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $root 'kit\coopplus\diagnose.ps1') -GameDir $bin -OutDir $t -NoClipboard -NoPause | Out-Null
$txt = Get-ChildItem $t -Filter 'CoopPlus-report-*.txt' | Select-Object -First 1
$zip = Get-ChildItem $t -Filter 'CoopPlus-report-*.zip' | Select-Object -First 1
Expand-Archive $zip.FullName (Join-Path $t 'unzipped')
$all = (Get-Content $txt.FullName -Raw) + (Get-Content (Join-Path $t 'unzipped\cdmp.log') -Raw)
$fails = 0
foreach ($s in $secrets) {
    if ($s -and $all.Contains($s)) { Write-Host "LEAK  $s" -ForegroundColor Red; $fails++ } else { Write-Host "gone  $s" }
}
foreach ($keep in '1.0.0.3479', '-10300.12', '127.0.0.1') {
    if ($all.Contains($keep)) { Write-Host "kept  $keep" } else { Write-Host "LOST  $keep" -ForegroundColor Red; $fails++ }
}
Write-Host '--- the redacted log:'
Get-Content (Join-Path $t 'unzipped\cdmp.log')
Remove-Item -Recurse -Force $t
if ($fails) { exit 1 }
