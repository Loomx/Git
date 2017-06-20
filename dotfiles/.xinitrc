#!/bin/sh
#
# ~/.xinitrc

#synclient TapButton1=0
#synclient TapButton2=0
#synclient TapButton3=0
#synclient HorizTwoFingerScroll=1

#xv -root -quit ~/Git/kdm/wallpaper.jpg
#xv -root -quit ~/Git/kdm/wallpaper-4x3.jpg
#redshift -l -43.5:172.6 &

setxkbmap -option ctrl:nocaps,compose:prsc

xcape

dstatus &

exec dwm 2>/dev/null
