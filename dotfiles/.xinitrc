#!/bin/sh
#
# ~/.xinitrc

xrdb -merge ~/.Xresources

xmodmap ~/.Xmodmap

dstatus &

exec dwm 2>/dev/null
