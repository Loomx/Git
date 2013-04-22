#!/bin/sh

echo "\
xterm*font:     -*-terminus-medium-r-*-*-16-*-*-*-*-*-*-*
xterm*reverseVideo:     true
xterm*titeInhibit:      true
xterm*tiXtraScroll:     true" | xrdb

xmodmap -e "keycode 133 = XF86Launch1" -e "keycode 134 = XF86Launch2"
xmodmap -e "clear lock" -e "keycode 66 = Control_R" -e "add Control = Control_R"

dwmstatus &

exec dwm 2>/dev/null
