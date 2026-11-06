# Native base on 1.0.0.3549: two copies on this PC, host + guest (ByJunter's option D), first join.
# Called by twocopy.sh (test_guard.sh first, cleanup after). Saves:
#  - host: everything redirected into %LOCALAPPDATA%\Pearl Abyss\CD\cdmp_save_sandbox (a copy made at start), the
#    test slot and a windowed settings file copied into the sandbox only (CDMP_SANDBOX_ADD);
#  - guest: the guest role sends the whole save folder to ...\CD\save_cdmp_guest, which this script fills with a test
#    slot and a windowed settings file. The real save folder is only read (its settings file, as the template).
param([string]$Name = 'tc1', [string]$HostVariant = 'coop-test-bandits', [string]$GuestVariant = 'rps',
      [int]$LoadWait = 80, [int]$JoinWait = 120, [int]$Hold = 60, [switch]$Walk, [switch]$Ride)
$ErrorActionPreference = 'Stop'
$bin = 'D:\SteamLibrary\steamapps\common\Crimson Desert\bin64'
$here = $PSScriptRoot; $saves = 'C:\Users\asher\CrimsonDesert-SeamlessCoop\build\test-saves'
$shot = 'C:\Users\asher\CrimsonDesert-SeamlessCoop\build\port3460\shot.ps1'
$cd = "$env:LOCALAPPDATA\Pearl Abyss\CD"; $acct = '1160582305'
if (Get-Process CrimsonDesert -ErrorAction SilentlyContinue) { throw 'Crimson Desert is running: test not started' }
if (-not (Get-Process steam -ErrorAction SilentlyContinue)) { throw 'Steam is not running' }
Copy-Item 'C:\Users\asher\CrimsonDesert-SeamlessCoop\upstream\cdmp-native\build\bin\RelWithDebInfo\cdmp.asi' "$bin\cdmp.asi" -Force

function Write-Ini([string]$path, [string]$text) {
    [IO.File]::WriteAllText($path, $text.Replace("`r`n", "`n").Replace("`n", "`r`n"), (New-Object Text.UnicodeEncoding($false, $true)))
}
Write-Ini "$bin\cdmp.host.ini" @'
[general]
player_name=Host
background_full_speed=1
[net]
enabled=1
transport=udp
role=host
peer_port=37015
host_loopback_only=1
[overlay]
enabled=0
[sniffer]
enabled=1
record=1
[coop]
enabled=0
[session2]
enabled=1
probe=1
save_guard=redirect
host_save=sandbox
create_session=1
accept_tunnel=1
share_field=1
relay_fix=1
clock_gate=1
identity_fix=1
child_hold=1
observer_hold=1
observer_load_defer=1
observer_hold_family=1
observer_hold_acks=1
character_hold=1
child_settle_ms=4000
child_reannounce=1
observer_seq_copy=1
observer_seq_copy_2322=1
guest_own_save=1
ui_pause_block=1
remote_interp=1
mount_sync=1
mount_seat=1
mount_relink=1
ride_filter=1
ride_dismount=native
horse_follow=1
horse_follow_reverse=1
user2_fast_travel=1
'@
Write-Ini "$bin\cdmp.guest.ini" @'
[general]
player_name=Guest
background_full_speed=1
[net]
enabled=1
transport=udp
role=guest
peer_host=127.0.0.1
peer_port=37015
[overlay]
enabled=0
[sniffer]
enabled=1
record=1
[coop]
enabled=0
[session2]
enabled=1
guest=1
save_guard=redirect
guest_mode=remote
guest_crash_dump=1
guest_layer_guard=1
guest_remote_update_guard=1
guest_update_guard=1
guest_actor_diag=1
guest_reload_wait_fix=1
guest_own_save=1
ui_pause_block=1
remote_interp=1
mount_sync=1
mount_seat=1
mount_relink=1
ride_filter=1
ride_dismount=native
guest_fast_travel_veto=probe
'@

# The windowed settings (template: the player's own file, read only).
$xml = [IO.File]::ReadAllText("$cd\save\user_engine_option_save.xml")
$win = $xml -replace '(<EnumSelectDisplayType Name="_displayType" _select=")[^"]*"', '$1Windowed"'
if ($win -eq $xml -and $xml -notmatch '_select="Windowed"') { throw 'display type not found in the settings file' }
# Two copies on one GPU: the lowest quality in both test copies (the player's own settings file is never written).
$win = $win -replace '(<EnumSelectQualityLevel Name="\w+" _select=")[^"]*"', '$1Minimum"'
$win = $win -replace '(<OptionStringVector Name="_postProcessingQualityLevelSelect" _value=")[^"]*"', '$1Minimum"'
$win = $win -replace '(<EnumSelectResolutionScale Name="_upscaleResolution" _select=")[^"]*"', '$1 50%"' -replace '_select=" 50%"', '_select="50%"'
[IO.File]::WriteAllText("$here\settings-windowed.xml", $win)

# The guest's own save folder (only ever made by this script; marked).
$g = "$cd\save_cdmp_guest"
if ((Test-Path $g) -and -not (Test-Path "$g\.coopplus-test")) { throw "$g exists and is not ours: left alone, test not started" }
if (Test-Path $g) { Remove-Item $g -Recurse -Force }
New-Item -ItemType Directory "$g\$acct" -Force | Out-Null
Set-Content "$g\.coopplus-test" 'made by build/native/twocopy.ps1'
Copy-Item "$saves\slot103-$GuestVariant" "$g\$acct\slot103" -Recurse
Copy-Item "$here\settings-windowed.xml" "$g\user_engine_option_save.xml"
Copy-Item "$cd\save\initialize_input.save" "$g\initialize_input.save"

Remove-Item "$bin\cdmp.host.log", "$bin\cdmp.guest.log" -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force "$here\shots" | Out-Null
$env:SteamAppId = '3321460'; $env:SteamGameId = '3321460'
function Start-Copy([string]$role, [string]$add) {
    $env:CDMP_ROLE = $role; $env:CDMP_SANDBOX_ADD = $add
    $env:CDMP_SANDBOX_ONLY = $(if ($role -eq 'host') { "$acct\slot103" } else { '' })
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
Remove-Item "$here\$Name.pids" -ErrorAction SilentlyContinue
$h = Start-Copy 'host' "$saves\slot103-$HostVariant|$acct\slot103;$here\settings-windowed.xml|user_engine_option_save.xml"
$h = $h | Where-Object { $_ -is [System.Diagnostics.Process] }
& $shot -Out "$here\shots\$Name-00-host-title.png" | Out-Null
& "$here\keyspid.ps1" -GamePid $h.Id -Keys E | Out-Null
Start-Sleep $LoadWait
& $shot -Out "$here\shots\$Name-10-host-loaded.png" | Out-Null

$gp = Start-Copy 'guest' ''
$gp = $gp | Where-Object { $_ -is [System.Diagnostics.Process] }
& $shot -Out "$here\shots\$Name-20-guest-title.png" | Out-Null
& "$here\keyspid.ps1" -GamePid $gp.Id -Keys E | Out-Null
Start-Sleep 10
# A second press if the first only refreshed the slot list (tc7): harmless once the load has started.
if (-not (Select-String -Path "$bin\cdmp.guest.log" -Pattern 'LoginCharacterToFieldServerReq' -Quiet)) {
    & "$here\keyspid.ps1" -GamePid $gp.Id -Keys E | Out-Null
}
Start-Sleep $JoinWait
& $shot -Out "$here\shots\$Name-30-guest-joined.png" | Out-Null
if ($Walk) {
    # Movement sync (tc23+): both spawn on one spot facing the same way (same save). The guest walks ahead (4 s of W)
    # and the host's window is shot (the guest should stand ahead of its camera); then the host walks 7 s (past the
    # guest) and the guest's window is shot. The packet logs give each side's positions (sent Req / received Ack).
    # (tc22: walk_to.py's steering wandered at 12 m; straight walks need no steering.)
    try {
        "guest walks at $(Get-Date -Format HH:mm:ss.fff)"
        & "$here\actpid.ps1" -GamePid $gp.Id -Seq "kd:W w:4000 ku:W w:1500 shot:$Name-33-guest-walked" | Out-Null
        & "$here\actpid.ps1" -GamePid $h.Id -Seq "w:2500 shot:$Name-34-host-sees-guest" | Out-Null
        "host walks at $(Get-Date -Format HH:mm:ss.fff)"
        & "$here\actpid.ps1" -GamePid $h.Id -Seq "kd:W w:7000 ku:W w:1500 shot:$Name-35-host-walked" | Out-Null
        & "$here\actpid.ps1" -GamePid $gp.Id -Seq "w:2500 shot:$Name-36-guest-sees-host" | Out-Null
    } catch { "walk step failed: $_" }
    "walks done $(Get-Date -Format HH:mm:ss.fff)"
    if ($Ride) {
        # Mounts (tc28+): the host calls its horse (H, the default key), mounts (F), rides forward and dismounts (F);
        # the guest's window is shot while the host rides. MountSync / HorseFollow lines in both logs.
        try {
            "host calls its horse at $(Get-Date -Format HH:mm:ss.fff)"
            & "$here\actpid.ps1" -GamePid $h.Id -Seq "k:H:150 w:12000 shot:$Name-38-host-horse k:F:150 w:5000 shot:$Name-39-host-mounted" | Out-Null
            & "$here\actpid.ps1" -GamePid $gp.Id -Seq "w:2500 shot:$Name-40-guest-sees-mounted" | Out-Null
            "host rides at $(Get-Date -Format HH:mm:ss.fff)"
            & "$here\actpid.ps1" -GamePid $h.Id -Seq "kd:W w:5000 ku:W w:2000 shot:$Name-41-host-rode" | Out-Null
            & "$here\actpid.ps1" -GamePid $gp.Id -Seq "w:2500 shot:$Name-42-guest-sees-rider" | Out-Null
            "host dismounts at $(Get-Date -Format HH:mm:ss.fff)"
            & "$here\actpid.ps1" -GamePid $h.Id -Seq "k:F:150 w:4000 shot:$Name-43-host-dismounted" | Out-Null
            & "$here\actpid.ps1" -GamePid $gp.Id -Seq "w:2500 shot:$Name-44-guest-sees-dismounted" | Out-Null
        } catch { "ride step failed: $_" }
        "ride done $(Get-Date -Format HH:mm:ss.fff)"
    }
    # UI pause block: the host opens its menu for 12 s; its world (and the guest's) must keep running.
    "host ESC at $(Get-Date -Format HH:mm:ss.fff)"
    & "$here\keyspid.ps1" -GamePid $h.Id -Keys ESC | Out-Null
    Start-Sleep 6
    & $shot -Out "$here\shots\$Name-37-host-menu.png" | Out-Null
    Start-Sleep 6
    & "$here\keyspid.ps1" -GamePid $h.Id -Keys ESC | Out-Null
    "host ESC closed at $(Get-Date -Format HH:mm:ss.fff)"
}
& "$here\keyspid.ps1" -GamePid $h.Id -Keys SPACE | Out-Null
Start-Sleep 3
& $shot -Out "$here\shots\$Name-31-host.png" | Out-Null
Start-Sleep $Hold
& $shot -Out "$here\shots\$Name-40-later.png" | Out-Null
"host alive=$(-not $h.HasExited) guest alive=$(-not $gp.HasExited)"
if ($gp.HasExited) { "guest exit code 0x{0:X8} at {1}" -f $gp.ExitCode, $gp.ExitTime }
if ($h.HasExited) { "host exit code 0x{0:X8} at {1}" -f $h.ExitCode, $h.ExitTime }
