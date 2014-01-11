#!/bin/sh
#
# ~/.xinitrc

xrdb -merge ~/.Xresources

xmodmap ~/.Xmodmap

dwmstatus &

exec dwm 2>/dev/null
