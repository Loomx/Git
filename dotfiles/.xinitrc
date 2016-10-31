#!/bin/sh
#
# ~/.xinitrc

#synclient TapButton1=0
#synclient TapButton2=0
#synclient TapButton3=0
#synclient HorizTwoFingerScroll=1

#xv -root -quit ~/Git/kdm/wallpaper.jpg
#xv -root -quit ~/Git/kdm/wallpaper-4x3.jpg

setxkbmap -option ctrl:nocaps,compose:prsc

xcape

dstatus &

exec dwm 2>/dev/null
