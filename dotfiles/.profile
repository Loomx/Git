#### ~/.profile ####

. ~/.bashrc

[ $(tty) = /dev/tty1 ] && exec xinit
