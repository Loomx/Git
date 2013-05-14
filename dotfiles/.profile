#### ~/.profile ####

. ~/.bashrc

[ $(tty) = /dev/tty1 ] && exec startx
