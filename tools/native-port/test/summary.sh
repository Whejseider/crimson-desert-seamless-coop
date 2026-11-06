#!/bin/bash
# One run's essentials: exit codes, guest crash sites, the host's join / hold lines, errors (no periodic status).
N=${1:?run name}; D=/c/Users/asher/CrimsonDesert-SeamlessCoop/build/native
echo "== guest"; grep -a "first-chance AV\| ERR \|GuestS3 guard\|move null skips" $D/$N.guest.log | grep -av "hooks on (remote)" | cut -c1-300 | tail -12
echo "== host"; grep -a " ERR \| WRN \|E3b H[12]\|logout asked\|[Oo]bserver hold ON\|child hold ON\|character hold" $D/$N.host.log | grep -av "save guard REDIRECT (to\|probe (minute" | cut -c1-230 | head -25
