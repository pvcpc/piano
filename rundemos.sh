#!/bin/sh

for demo in $(find demos -type f -executable); do
	echo "[$0] Heads up! Demo '$demo' up next in 1 second!"
	sleep 1
	"$demo"
done
