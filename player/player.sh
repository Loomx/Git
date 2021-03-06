#!/bin/sh
#
# /usr/local/bin/player
#
# Depends: dmenu, mplayer, inotifywait


# Cache files:
AlbumCache=~/.mplayer/album_cache
TrackCache=~/.mplayer/track_cache

# Update caches if anything has changed:
if stest -dlq -n "$AlbumCache" ~/Music; then
    stest -dl ~/Music | sort >"$AlbumCache"
    stest -f ~/Music/*/* >"$TrackCache"
fi

# Set trap to clean up afterwards:
clean_up () { rm /tmp/mp_output /tmp/status_msg; }
trap 'clean_up' EXIT

# Select an album:
Album="$( { printf "Jukebox\nDVD\n"; cat "$AlbumCache"; } | dmenu -i -l 40 )" || exit

case "$Album" in
    Jukebox)
        Filters="$( dmenu </dev/null -p "Filters?" )" || exit

        if [ "$Filters" ]; then
            # Apply filters and shuffle tracks:
            set $( echo "$Filters" ) 
            for i; do 
                grep -Fi "$i" "$TrackCache"
            done | mplayer -playlist - -shuffle -identify >/tmp/mp_output &
        else
            # No filters, so shuffle all tracks:
            mplayer -playlist "$TrackCache" -shuffle -identify >/tmp/mp_output &
        fi
        ;;

    DVD)
        exec mplayer dvd://
        ;;

    *)
        # Album is selected, prompt to play, shuffle or choose track:
        Track="$( { printf "Play\nShuffle\n"; stest -fl ~/Music/"$Album" | sort; } | dmenu -i -l 40 )" || exit

        case "$Track" in
            Play)
                mplayer ~/Music/"$Album"/* -identify >/tmp/mp_output &
                ;;

            Shuffle)
                mplayer ~/Music/"$Album"/* -shuffle -identify >/tmp/mp_output &
                ;;

            *)
                mplayer ~/Music/"$Album"/"$Track" -identify >/tmp/mp_output &
                ;;
        esac
        ;;
esac

# Pause while mplayer starts:
sleep 1

# Loop while playing to store current track name:
while pgrep mplayer >/dev/null; do
    tail -20 /tmp/mp_output | awk -F "/" '/FILENAME/ { print $NF }' >/tmp/status_msg
    inotifywait -q -e modify /tmp/mp_output >/dev/null
done
