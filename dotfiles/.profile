# ~/.profile

. ~/.bashrc

if [ -z "$DISPLAY" -a $(tty) = /dev/tty1 ]; then
    exec xinit -- -nolisten tcp
fi
