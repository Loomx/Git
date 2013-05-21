#### ~/.bashrc ####

[ -z "$PS1" ] && return

. /usr/share/bash-completion/bash_completion
CDPATH=".:..:~:/"
shopt -s autocd cdspell checkwinsize histappend
HISTCONTROL=erasedups
set -o vi

PROMPT_COMMAND='[ "$PWD" != "$Prev" ] && ls --color; Prev="$PWD"'
PS1='\[\033[0;34m\]\u \[\033[0;33m\]\w \[\033[m\]'  # blue=34 red=31 green=32

bind '"\t":menu-complete'
bind '"\e[A":history-search-backward'
bind '"\e[B":history-search-forward'
bind '"\C-l":clear-screen'

l () { ls --color $*; }
la () { ls -A --color $*; }
ll () { ls -oh --color $*; }
lla () { ls -ohA --color $*; }
usbin () {
    [ -d ~/USB ] || mkdir ~/USB
    mount ~/USB && cd ~/USB || rmdir ~/USB
}
usbout () {
    [[ "$PWD" == ~/USB* ]] && cd ~
    umount ~/USB && rmdir ~/USB
}
phonein () {
    [ -d ~/PHONE ] || mkdir ~/PHONE
    mount ~/PHONE && cd ~/PHONE || rmdir ~/PHONE
}
phoneout () {
    [[ "$PWD" == ~/PHONE* ]] && cd ~
    umount ~/PHONE && rmdir ~/PHONE
}
sdin () {
    [ -d ~/SD ] || mkdir ~/SD
    mount ~/SD && cd ~/SD || rmdir ~/SD
}
sdout () {
    [[ "$PWD" == ~/SD* ]] && cd ~
    umount ~/SD && rmdir ~/SD
}
backup () {
    cd ~ && rsync -a --delete \
    --exclude=Git/ --exclude=Music/ --exclude=.mozilla/ \
    --exclude=USB/ --exclude=PHONE/ --exclude=SD/ \
    ~/ ~/USB/backup/
}
tao () {
    awk -v verse=$( echo $[RANDOM%80+2] ) 'RS=""; NR==verse' ~/Documents/tao.txt
}

