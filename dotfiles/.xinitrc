#!/bin/sh
#
# ~/.xinitrc

#synclient TapButton1=0
#synclient TapButton2=0
#synclient TapButton3=0
#synclient HorizTwoFingerScroll=1

display -window root Git/dotfiles/wallpaper.jpg
#xrandr --output eDP-1 --auto --gamma 1:1:0.75

setxkbmap -option ctrl:nocaps,compose:prsc

xcape

dstatus &

xrdb ~/.Xresources

export GTK_OVERLAY_SCROLLING=0

if [ -z "$DBUS_SESSION_BUS_ADDRESS"]; then
	eval $(dbus-launch --sh-syntax --exit-with-session)
fi

killall pipewire pipewire-media-session pipewire-pulse
pipewire &
pipewire-media-session &
pipewire-pulse &

exec dwm 2>/dev/null
