# Native base (ByJunter's NativeServer, ported to 1.0.0.3549): one copy, host role, read-only session2 probe.
# Called by probe.sh (which sources test_guard.sh first and cleans up after). The host role redirects every save
# into %LOCALAPPDATA%\Pearl Abyss\CD\cdmp_save_sandbox from the start; the test slot is copied into the sandbox
# only (CDMP_SANDBOX_ADD), never into the real save folder.
param([string]$Name = 'np1', [string]$Variant = 'coop-test-bandits', [int]$LoadWait = 75, [int]$Hold = 80)
$ErrorActionPreference = 'Stop'
$bin = 'D:\SteamLibrary\steamapps\common\Crimson Desert\bin64'
$here = $PSScriptRoot; $keys = 'C:\Users\asher\CrimsonDesert-SeamlessCoop\build\port3460'
if (Get-Process CrimsonDesert -ErrorAction SilentlyContinue) { throw 'Crimson Desert is running: test not started' }
if (-not (Get-Process steam -ErrorAction SilentlyContinue)) { throw 'Steam is not running' }
$asi = 'C:\Users\asher\CrimsonDesert-SeamlessCoop\upstream\cdmp-native\build\bin\RelWithDebInfo\cdmp.asi'
Copy-Item $asi "$bin\cdmp.asi" -Force
$ini = @'
[general]
player_name=Host
[net]
enabled=0
[overlay]
enabled=0
[sniffer]
enabled=0
[coop]
enabled=0
[session2]
enabled=1
probe=1
save_guard=redirect
host_save=sandbox
create_session=0
'@
[IO.File]::WriteAllText("$bin\cdmp.host.ini", $ini.Replace("`n", "`r`n"), (New-Object Text.UnicodeEncoding($false, $true)))
Remove-Item "$bin\cdmp.host.log" -ErrorAction SilentlyContinue
$env:CDMP_ROLE = 'host'; $env:SteamAppId = '3321460'; $env:SteamGameId = '3321460'
$env:CDMP_SANDBOX_ADD = "C:\Users\asher\CrimsonDesert-SeamlessCoop\build\test-saves\slot103-$Variant|1160582305\slot103"
$p = Start-Process "$bin\CrimsonDesert.exe" -ArgumentList 'PlatformServiceType=Steam' -WorkingDirectory $bin -PassThru
"started pid=$($p.Id)"
Set-Content "$here\$Name.pid" $p.Id
New-Item -ItemType Directory -Force "$here\shots" | Out-Null
$t0 = Get-Date
while (((Get-Date) - $t0).TotalSeconds -lt 120) {
    Start-Sleep 2
    if ($p.HasExited) { "game exited early (code $($p.ExitCode))"; exit 2 }
    if ((Test-Path "$bin\cdmp.host.log") -and (Select-String -Path "$bin\cdmp.host.log" -Pattern 'Late init: game is up' -Quiet)) { break }
}
Start-Sleep 20
& "$keys\shot.ps1" -Out "$here\shots\$Name-00-title.png" | Out-Null
& "$keys\keys.ps1" -Keys E | Out-Null
Start-Sleep $LoadWait
& "$keys\shot.ps1" -Out "$here\shots\$Name-10-loaded.png" | Out-Null
& "$keys\keys.ps1" -Keys SPACE -HoldMs 1500 | Out-Null
Start-Sleep $Hold
& "$keys\shot.ps1" -Out "$here\shots\$Name-20-later.png" | Out-Null
"alive=$(-not $p.HasExited)"
