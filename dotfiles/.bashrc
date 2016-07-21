#### ~/.bashrc ####

[ -z "$PS1" ] && return

if [ $UID -eq 0 ]; then
	. /etc/profile
	PS1='\[\e[0;31m\][ \w ]\n\[\e[0;31m\]\h\$ \[\e[m\]'
else
	PS1='\[\e[0;33m\][ \w ]\n\[\e[0;33m\]\h\$ \[\e[m\]'
fi

PROMPT_COMMAND='[ "$PWD" != "$Prev" ] && ls --color --group-directories-first; Prev="$PWD"'

. /etc/profile.d/bash_completion.sh
CDPATH=".:..:~"
shopt -s autocd cdspell checkwinsize #histappend
HISTCONTROL=erasedups
HISTSIZE=5000
HISTFILESIZE=5000
set -o vi

bind '"\t":menu-complete'
bind '"\e[Z":menu-complete-backward'
bind -m vi-command '"k":history-search-backward'
bind -m vi-command '"j":history-search-forward'
bind '"\e[A":history-search-backward'
bind '"\e[B":history-search-forward'
bind '"\C-a":beginning-of-line'
bind '"\C-e":end-of-line'
bind '"\C-l":"clear; histfix; ls --color --group-directories-first\n"'

l () { ls --color --group-directories-first $*; }
la () { ls -A --color --group-directories-first $*; }
ll () { ls -lh --color --group-directories-first $*; }
lla () { ls -lha --color --group-directories-first $*; }

man() {
	env LESS_TERMCAP_md=$'\e[34m' \
	LESS_TERMCAP_me=$'\e[0m' \
	LESS_TERMCAP_so=$'\e[32m' \
	LESS_TERMCAP_se=$'\e[0m' \
	LESS_TERMCAP_us=$'\e[33m' \
	LESS_TERMCAP_ue=$'\e[0m' \
	man "$@"
}

# mount/unmount helper functions
localmount () {
	mkdir ~/"$1" 2>/dev/null
	mount ~/"$1" && cd ~/"$1" || rmdir ~/"$1"
}
localumount () {
	[[ "$PWD" == ~/"$1"* ]] && cd ~
	umount ~/"$1"; rmdir ~/"$1"
}

# generic mounts
usbin () { localmount USB; }
usbout () { localumount USB; }
sdcardin () { localmount SDCARD; }
sdcardout () { localumount SDCARD; }

# specific device mounts
phonein () { localmount PHONE; }
phoneout () { localumount PHONE; }
sdin () { localmount SD; }
sdout () { localumount SD; }
datain () { localmount DATA; }
dataout () { localumount DATA; }

backup-to-phone () {
	if [ -d ~/PHONE/Internal\ storage/Linux/.backup ]; then
		rsync -axx --delete \
		--exclude=Git/ --exclude=Music/ --exclude=.cache/ \
		~/ ~/PHONE/Internal\ storage/Linux/.backup/$(hostname)/
	fi
}
tao () {
	awk -v verse=$((RANDOM%80+2)) 'RS=""; NR==verse' ~/Documents/tao.txt
}
hdmiin () {
	xrandr --auto --output HDMI1 --mode 1280x800
	xset -dpms; xset s off
	cp ~/.asoundrc-hdmi ~/.asoundrc
	#pactl set-card-profile 0 output:hdmi-stereo
}
hdmiout () {
	xset +dpms; xset s default
	rm ~/.asoundrc
	#pactl set-card-profile 0 output:analog-stereo
}
histfix () {
	history -a
	tac ~/.bash_history | awk '!x[$0]++' | tac >~/hist.tmp && \
	mv ~/hist.tmp ~/.bash_history
}

trap histfix EXIT

[ -r ~/.bashrc_local ] && . ~/.bashrc_local
