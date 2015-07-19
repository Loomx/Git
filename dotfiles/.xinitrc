#!/bin/sh
#
# ~/.xinitrc

setxkbmap -option ctrl:nocaps

xcape

dstatus &

exec dwm 2>/dev/null
