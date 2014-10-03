#!/bin/sh
#
# ~/.xinitrc

xrdb -merge ~/.Xresources

setxkbmap -option caps:escape

dstatus &

exec dwm 2>/dev/null
