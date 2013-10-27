#### ~/.bashrc ####

[ -z "$PS1" ] && return

[ "$USER" = "root" ] && . /etc/profile

. /etc/profile.d/bash_completion.sh
CDPATH=".:..:~:/"
shopt -s autocd cdspell checkwinsize histappend
HISTCONTROL=erasedups
unset HISTFILESIZE
set -o vi

# Remove duplicates from ~/.bash_history
tac ~/.bash_history | awk '!a[$0]++' | tac >/tmp/hist && mv /tmp/hist ~/.bash_history

PROMPT_COMMAND='[ "$PWD" != "$Prev" ] && ls --color; Prev="$PWD"; history -a'

if [ "$USER" = "root" ]; then
    PS1='\[\033[0;31m\]\u \[\033[0;33m\]\w \[\033[m\]'
else
    PS1='\[\033[0;34m\]\u \[\033[0;33m\]\w \[\033[m\]'
fi

bind '"\t":menu-complete'
bind '"\e[A":history-search-backward'
bind '"\e[B":history-search-forward'
bind '"\C-l":clear-screen'
bind '"\C-a":beginning-of-line'
bind '"\C-e":end-of-line'

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
    ~/ ~/USB/backup/
}
tao () {
    awk -v verse=$( echo $[RANDOM%80+2] ) 'RS=""; NR==verse' ~/Documents/tao.txt
}

