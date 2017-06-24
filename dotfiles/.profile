# ~/.profile

. ~/.bashrc

if [ "$(tty)" = "/dev/tty1" ]; then
    exec xinit -- -nolisten tcp
fi
