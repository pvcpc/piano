#!/bin/sh

PRE_SLEEP=${1:-3}
POST_SLEEP=${2:-3}
DEMOS=$(find demos -type f -executable)

NUM_DEMO=0
TOT_DEMOS=$(echo "$DEMOS" | wc -l)

for demo in $DEMOS; do
	NUM_DEMO=$((NUM_DEMO + 1))
	clear
	printf "[$0] Heads up! Demo \033[1m\033[92m$demo\033[0m ($NUM_DEMO/$TOT_DEMOS) up next in $PRE_SLEEP second(s)!"
	sleep "$PRE_SLEEP"
	"$demo"
	sleep "$POST_SLEEP"
done

clear
echo "[$0] All $TOT_DEMOS demos finished!"
