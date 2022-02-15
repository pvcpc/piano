#!/bin/sh

SLEEP=${1:-1}

for demo in $(find demos -type f -executable); do
	echo "[$0] Heads up! Demo '$demo' up next in $SLEEP second(s)!"
	sleep "$SLEEP"
	"$demo"
done
