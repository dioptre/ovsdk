#!/bin/bash

for f in *.png; do
	is_dest=`echo $f | grep offscreen`
	if [ "$is_dest" = "" ]; then
		echo working on $f...
		convert "$f" -sepia-tone 90% -modulate 45% "$f-offscreen"
	fi
done

