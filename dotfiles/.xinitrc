#!/bin/sh

echo "\
! black
*color0:                  #073642
*color8:                  #002b36
! red
*color1:                  #dc322f
*color9:                  #cb4b16
! green
*color2:                  #859900
*color10:                 #859900
! yellow
*color3:                  #b58900
*color11:                 #b58900
! blue
*color4:                  #268bd2
*color12:                 #268bd2
! magenta
*color5:                  #d33682
*color13:                 #6c71c4
! cyan
*color6:                  #2aa198
*color14:                 #2aa198
! white
*color7:                  #eee8d5
*color15:                 #fdf6e3
!
*background:              #002b36
*foreground:              #839496
*fading:                  40
*fadeColor:               #002b36
*cursorColor:             #93a1a1
*pointerColorBackground:  #586e75
*pointerColorForeground:  #93a1a1
!
xterm*font:     -*-terminus-medium-r-*-*-16-*-*-*-*-*-*-*
xterm*titeInhibit:      true
xterm*tiXtraScroll:     true" | xrdb

xmodmap -e "keycode 133 = XF86Launch1" -e "keycode 134 = XF86Launch2"
xmodmap -e "clear lock" -e "keycode 66 = Control_R" -e "add Control = Control_R"

dwmstatus &

exec dwm 2>/dev/null
