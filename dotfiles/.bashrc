#### ~/.bashrc ####

[ -z "$PS1" ] && return

[ "$USER" = "root" ] && . /etc/profile

. /etc/profile.d/bash_completion.sh
CDPATH=".:..:~:/"
shopt -s autocd cdspell checkwinsize #histappend
HISTCONTROL=erasedups
set -o vi

PROMPT_COMMAND='[ "$PWD" != "$Prev" ] && ls --color; Prev="$PWD"'

if [ "$USER" = "root" ]; then
    PS1='\[\e[0;31m\][ \w ]\n\[\e[0;31m\]\$ \[\e[m\]'
else
    PS1='\[\e[0;33m\][ \w ]\n\[\e[0;33m\]\$ \[\e[m\]'
fi

bind '"\t":menu-complete'
bind '"\e[A":history-search-backward'
bind '"\e[B":history-search-forward'
bind '"\C-a":beginning-of-line'
bind '"\C-e":end-of-line'
bind '"\C-l":"clear; ls --color\n"'

l () { ls --color $*; }
la () { ls -A --color $*; }
ll () { ls -lh --color $*; }
lla () { ls -lha --color $*; }
usbin () {
    mkdir ~/USB 2>/dev/null
    mount ~/USB && cd ~/USB || rmdir ~/USB
}
usbout () {
    [[ "$PWD" == ~/USB* ]] && cd ~
    umount ~/USB; rmdir ~/USB
}
phonein () {
    mkdir ~/PHONE 2>/dev/null
    mount ~/PHONE && cd ~/PHONE || rmdir ~/PHONE
}
phoneout () {
    [[ "$PWD" == ~/PHONE* ]] && cd ~
    umount ~/PHONE; rmdir ~/PHONE
}
sdin () {
    mkdir ~/SD 2>/dev/null
    mount ~/SD && cd ~/SD || rmdir ~/SD
}
sdout () {
    [[ "$PWD" == ~/SD* ]] && cd ~
    umount ~/SD; rmdir ~/SD
}
backup () {
    cd ~ && rsync -a --delete \
    --exclude=USB/ --exclude=PHONE/ --exclude=SD/ \
    --exclude=Git/ --exclude=Music/ --exclude=.cache/ \
    ~/ ~/PHONE/Linux/.backup/
}
tao () {
    awk -v verse=$( echo $[RANDOM%80+2] ) 'RS=""; NR==verse' ~/Documents/tao.txt
}
hdmiin () {
    xrandr --auto --output HDMI1 --mode 1280x800
    cp ~/.asoundrc-hdmi ~/.asoundrc
}
hdmiout () {
    rm ~/.asoundrc
}
histfix () {
    tac ~/.bash_history | awk '!x[$0]++' | tac >/tmp/hist && mv /tmp/hist ~/.bash_history
}
trap histfix EXIT
