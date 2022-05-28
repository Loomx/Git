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
shopt -s autocd cdspell checkwinsize globstar
HISTCONTROL=erasedups
HISTSIZE=20000
HISTFILESIZE=20000
set -o vi

bind '"\t":menu-complete'
bind '"\e[Z":menu-complete-backward'
bind -m vi-command '"k":history-search-backward'
bind -m vi-command '"j":history-search-forward'
bind '"\e[A":history-search-backward'
bind '"\e[B":history-search-forward'
bind '"\C-f":forward-search-history'
bind '"\C-]":insert-last-argument'
bind '"\C-k":kill-line'
bind '"\C-l":"clear; histfix; ls --color --group-directories-first\n"'

l () { ls --color --group-directories-first "$@"; }
la () { ls -A --color --group-directories-first "$@"; }
ll () { ls -lh --color --group-directories-first "$@"; }
lla () { ls -lha --color --group-directories-first "$@"; }

cd () { 
	case $1 in 
		"")   pushd $HOME >/dev/null ;;
		--)   pushd "$2" >/dev/null  ;;
		-)    pushd >/dev/null       ;;
		*)    pushd "$1" >/dev/null  ;;
	esac
}

.. () { pushd .. >/dev/null; }

d () { dest=$(dirs -v | awk '!seen[$2]++' | head | fzy) &&
	   eval cd ~${dest:1}; }

fd () { find . -type f -iname \*"$1"\*; }

man () {
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
	mkdir -p ~/"$1"
	mount ~/"$1" && cd ~/"$1" || rmdir ~/"$1"
}
localumount () {
	[[ "$PWD" == ~/"$1"* ]] && cd ~
	umount ~/"$1" && rmdir ~/"$1"
}

# generic mounts
usbin () { localmount USB; }
usbout () { localumount USB; }
sdcardin () { localmount SDCARD; }
sdcardout () { localumount SDCARD; }

# specific device mounts
datain () { localmount DATA; }
dataout () { localumount DATA; }
data2in () { localmount DATA2; }
data2out () { localumount DATA2; }

phone-list () { adb shell ls /sdcard/DCIM/Camera/; }
phone-get () { adb pull /sdcard/DCIM/Camera/; }
phone-upload () { adb push "$@" /sdcard/Download/; }

tao () { sed -n "/-$((RANDOM%81+1))-/,/^$/p" ~/Documents/tao.txt; }

histfix () {
	history -a
	tac ~/.bash_history | awk '!x[$0]++' | tac >~/hist.tmp && \
	mv ~/hist.tmp ~/.bash_history
	history -c; history -r
}

trap histfix EXIT

[ -r ~/.bashrc_local ] && . ~/.bashrc_local
