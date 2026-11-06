#!/bin/bash
# Native base, read-only host probe (probe.ps1), then the usual cleanup: the game closed, savecheck against the
# snapshot test_guard.sh made, the player's ini, the release asi back, the role ini removed.
source /c/Users/asher/CrimsonDesert-SeamlessCoop/build/port3479/test_guard.sh
N=/c/Users/asher/CrimsonDesert-SeamlessCoop/build/native; H=/c/Users/asher/CrimsonDesert-SeamlessCoop/build/port3479
BIN="/d/SteamLibrary/steamapps/common/Crimson Desert/bin64"
REL=/c/Users/asher/CrimsonDesert-SeamlessCoop/dist/CrimsonDesert-SeamlessCoop-0.1.0-alpha+a82c1b48/mod/cdmp.asi
NAME=${1:-np1}
powershell -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w $N/probe.ps1)" -Name $NAME ${PSARGS:-} 2>&1 < /dev/null
GPID=$(cat $N/$NAME.pid 2>/dev/null | tr -d '\r')
cp "$BIN/cdmp.host.log" $N/$NAME.log 2>/dev/null
[ -n "$GPID" ] && taskkill //PID $GPID //F >/dev/null 2>&1
# The game's own short-lived second process (a child of ours) too; never any other copy.
[ -n "$GPID" ] && powershell -NoProfile -Command "Get-CimInstance Win32_Process -Filter \"Name='CrimsonDesert.exe' AND ParentProcessId=$GPID\" | ForEach-Object { Stop-Process -Id \$_.ProcessId -Force }" >/dev/null 2>&1
sleep 6
cp "$BIN/cdmp.host.log" $N/$NAME.log 2>/dev/null
cp $REL "$BIN/cdmp.asi"; rm -f "$BIN/cdmp.host.ini"
bash $H/savecheck.sh --restore-ini; bash $H/savecheck.sh --fix | tail -2
grep -aE "Session2|save redirect|REDIRECT|error|ERROR|refus|Role" $N/$NAME.log | cut -c1-260 | head -60
