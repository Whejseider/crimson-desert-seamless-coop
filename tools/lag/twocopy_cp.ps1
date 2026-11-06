# Co-op+ (released mod, old base) with TWO copies of the game on this PC (ByJunter's way: SteamAppId / SteamGameId
# and PlatformServiceType=Steam), host + guest over UDP on loopback. Called by twocopy_cp.sh (test_guard.sh first,
# cleanup after). Saves: each copy's whole save folder goes to its own scratch world
# (%LOCALAPPDATA%\CrimsonDesert-CoopPlus\Worlds\twocopy_<role>, made here, marked), with a test slot and a windowed
# low-quality settings file; the real save folder is only read (its settings file, as the template). The role inis
# come from the player's ini snapshot (build\port3479\cdmp.ini.player) with the test settings on top.
param([string]$Name = 'cp1', [string]$HostVariant = 'coop-test-bandits', [string]$GuestVariant = 'coop-test-bandits',
      [int]$LoadWait = 75, [int]$Hold = 20, [string]$HostSeq = '', [string]$GuestSeq = '')
$ErrorActionPreference = 'Stop'
$bin = 'D:\SteamLibrary\steamapps\common\Crimson Desert\bin64'
$here = $PSScriptRoot; $native = 'C:\Users\asher\CrimsonDesert-SeamlessCoop\build\native'
$saves = 'C:\Users\asher\CrimsonDesert-SeamlessCoop\build\test-saves'
$shot = 'C:\Users\asher\CrimsonDesert-SeamlessCoop\build\port3460\shot.ps1'
$cd = "$env:LOCALAPPDATA\Pearl Abyss\CD"; $acct = '1160582305'
$worlds = "$env:LOCALAPPDATA\CrimsonDesert-CoopPlus\Worlds"
if (Get-Process CrimsonDesert -ErrorAction SilentlyContinue) { throw 'Crimson Desert is running: test not started' }
if (-not (Get-Process steam -ErrorAction SilentlyContinue)) { throw 'Steam is not running' }
$orig = 'C:\Users\asher\CrimsonDesert-SeamlessCoop\build\port3479\cdmp.ini.player'
if (-not (Test-Path $orig)) { throw 'no player ini snapshot (test_guard.sh makes it)' }
Copy-Item 'C:\Users\asher\CrimsonDesert-SeamlessCoop\upstream\cd-coop-3446\build\bin\RelWithDebInfo\cdmp.asi' "$bin\cdmp.asi" -Force

# The windowed, lowest-quality settings (template: the player's own file, read only).
$xml = [IO.File]::ReadAllText("$cd\save\user_engine_option_save.xml")
$win = $xml -replace '(<EnumSelectDisplayType Name="_displayType" _select=")[^"]*"', '$1Windowed"'
if ($win -eq $xml -and $xml -notmatch '_select="Windowed"') { throw 'display type not found in the settings file' }
$win = $win -replace '(<EnumSelectQualityLevel Name="\w+" _select=")[^"]*"', '$1Minimum"'
$win = $win -replace '(<OptionStringVector Name="_postProcessingQualityLevelSelect" _value=")[^"]*"', '$1Minimum"'
$win = $win -replace '(<EnumSelectResolutionScale Name="_upscaleResolution" _select=")[^"]*"', '$1 50%"' -replace '_select=" 50%"', '_select="50%"'

function New-World([string]$role, [string]$variant) {
    $w = "$worlds\twocopy_$role"
    if ((Test-Path $w) -and -not (Test-Path "$w\.coopplus-test")) { throw "$w exists and is not ours: left alone" }
    if (Test-Path $w) { Remove-Item $w -Recurse -Force }
    New-Item -ItemType Directory "$w\$acct" -Force | Out-Null
    Set-Content "$w\.coopplus-test" 'made by build/port3527/twocopy_cp.ps1'
    Copy-Item "$saves\slot103-$variant" "$w\$acct\slot103" -Recurse
    [IO.File]::WriteAllText("$w\user_engine_option_save.xml", $win)
    Copy-Item "$cd\save\initialize_input.save" "$w\initialize_input.save"
}
New-World 'host' $HostVariant
New-World 'guest' $GuestVariant

function Write-RoleIni([string]$role, [string[]]$set) {
    $lines = ([IO.File]::ReadAllText($orig, [Text.Encoding]::Unicode).TrimStart([char]0xFEFF)) -split "`r?`n"
    foreach ($s in $set) {
        $sec, $kv = $s.Split('.', 2); $k, $v = $kv.Split('=', 2)
        $cur = ''; $done = $false
        $lines = foreach ($l in $lines) {
            if ($l -match '^\[(.+)\]') { if ($cur -eq $sec -and -not $done) { "$k=$v"; $done = $true }; $cur = $Matches[1] }
            if ($cur -eq $sec -and $l -match "^$k=") { "$k=$v"; $done = $true } else { $l }
        }
        if (-not $done) {
            $i = [array]::IndexOf($lines, "[$sec]")
            if ($i -ge 0 -and $i -lt $lines.Count - 1) { $lines = $lines[0..$i] + "$k=$v" + $lines[($i + 1)..($lines.Count - 1)] }
            elseif ($i -ge 0) { $lines += "$k=$v" }
            else { $lines += "[$sec]"; $lines += "$k=$v" }
        }
    }
    [IO.File]::WriteAllText("$bin\cdmp.$role.ini", ($lines -join "`r`n"), (New-Object Text.UnicodeEncoding($false, $true)))
}
$common = @('net.enabled=1', 'net.transport=udp', 'net.peer_host=127.0.0.1', 'net.peer_port=37015', 'coop.enabled=1',
            'coop.offset_m=0', 'coop.coop_worlds=0', 'general.onboarded=1', 'general.experimental_asked=1',
            'sniffer.record=0', 'dev.lag_probe=1')
Write-RoleIni 'host' ($common + @('net.role=host', 'general.player_name=Host', 'dev.coop_world=twocopy_host'))
Write-RoleIni 'guest' ($common + @('net.role=guest', 'general.player_name=Guest', 'dev.coop_world=twocopy_guest'))

Remove-Item "$bin\cdmp.host.log", "$bin\cdmp.guest.log", "$bin\cdmp_lag.host.csv", "$bin\cdmp_lag.guest.csv" -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force "$here\shots" | Out-Null
$env:SteamAppId = '3321460'; $env:SteamGameId = '3321460'
function Start-Copy([string]$role) {
    $env:CDMP_ROLE = $role
    $p = Start-Process "$bin\CrimsonDesert.exe" -ArgumentList 'PlatformServiceType=Steam' -WorkingDirectory $bin -PassThru
    Add-Content "$here\$Name.pids" $p.Id
    "$role pid=$($p.Id)"
    $t0 = Get-Date
    while (((Get-Date) - $t0).TotalSeconds -lt 150) {
        Start-Sleep 2
        if ($p.HasExited) { "$role exited early (code $($p.ExitCode))"; exit 2 }
        if ((Test-Path "$bin\cdmp.$role.log") -and (Select-String -Path "$bin\cdmp.$role.log" -Pattern 'Late init: game is up' -Quiet)) { break }
    }
    Start-Sleep 20
    return $p
}
# Play (E) at the title; once more 10 s later if the first only woke the title (harmless while loading).
function Press-Play([System.Diagnostics.Process]$p) {
    & "$native\keyspid.ps1" -GamePid $p.Id -Keys E | Out-Null
    Start-Sleep 10
    & "$native\keyspid.ps1" -GamePid $p.Id -Keys E | Out-Null
}
Remove-Item "$here\$Name.pids" -ErrorAction SilentlyContinue
$h = Start-Copy 'host'; $h = $h | Where-Object { $_ -is [System.Diagnostics.Process] }
Press-Play $h
Start-Sleep $LoadWait
$g = Start-Copy 'guest'; $g = $g | Where-Object { $_ -is [System.Diagnostics.Process] }
Press-Play $g
Start-Sleep $LoadWait
& $shot -Out "$here\shots\$Name-10-both-loaded.png" | Out-Null
"linked: host '$((Select-String -Path "$bin\cdmp.host.log" -Pattern 'Co-op: connected|in our world' | Select-Object -First 2 | ForEach-Object { $_.Line.Substring(15) }) -join ' / ')'"
if ($HostSeq) { "host acts $(Get-Date -Format HH:mm:ss.fff)"; & "$native\actpid.ps1" -GamePid $h.Id -Seq $HostSeq | Out-Null }
if ($GuestSeq) { "guest acts $(Get-Date -Format HH:mm:ss.fff)"; & "$native\actpid.ps1" -GamePid $g.Id -Seq $GuestSeq | Out-Null }
Start-Sleep $Hold
& $shot -Out "$here\shots\$Name-20-end.png" | Out-Null
"done $(Get-Date -Format HH:mm:ss.fff)"
