/* dstatus
 * based on one by Trilby White
 * GPL Licence
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <alloca.h>
#include <alsa/asoundlib.h>
#include <alsa/control.h>
#include <X11/Xlib.h>

#define TRACK_FILE      "/tmp/status_msg"
#define MEM_FILE        "/proc/meminfo"
#define BAT_FILE        "/sys/class/power_supply/BAT0/capacity"
#define FIFO            "/tmp/mp_pipe"
#define NET_FILE        "/sys/class/net/wlan0/operstate"

#define LOW_BAT_LVL     5

int
main(void) {
	Display *dpy;
	int bat, cpid, fd, reset, vol;
	long lnum1, lnum2, lnum3, lnum4;
	char net[5], track[50], status[100], *str;
	time_t current;
	FILE *fp;
	snd_hctl_t *hctl;
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_value_t *control;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "ERROR: could not open display\n");
		exit(1);
	}

	if ((snd_hctl_open(&hctl, "hw:0", 0))) {
		fprintf(stderr, "ERROR: could not open soundcard\n");
		exit(2);
	}
	snd_hctl_load(hctl);
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(id, "Master Playback Volume");
	snd_hctl_elem_t *elem = snd_hctl_find_elem(hctl, id);
	snd_ctl_elem_value_alloca(&control);
	snd_ctl_elem_value_set_id(control, id);

	for (;;sleep(1)) {
		str = status;

		/* Track */
		if ((fp = fopen(TRACK_FILE, "r"))) {
			fgets(track, 49, fp);
			track[strcspn(track, "\n")] = 0;
			fclose(fp);
			str += sprintf(str, "%s   ", track);
		}

		/* Memory */
		if ((fp = fopen(MEM_FILE, "r"))) {
			fscanf(fp, "MemTotal: %ld kB\nMemFree: %ld kB\nMemAvailable: %*d kB\nBuffers: %ld kB\nCached: %ld kB\n",
			            &lnum1, &lnum2, &lnum3, &lnum4);
			fclose(fp);
			str += sprintf(str, "Mem:%ld  ", (lnum1-(lnum2+lnum3+lnum4))/(lnum1/100));
		}

		/* Volume */
		if (!(snd_hctl_elem_read(elem, control))) {
			vol = (int)snd_ctl_elem_value_get_integer(control,0);
			str += sprintf(str, "Vol:%d  ", vol);
		}

		/* Battery */
		if ((fp = fopen(BAT_FILE, "r"))) {
			fscanf(fp, "%d\n", &bat);
			fclose(fp);
			if (bat > LOW_BAT_LVL)
				reset = 0;
			if (bat <= LOW_BAT_LVL && reset == 0) {
				reset = 1;
				if ((fd = open(FIFO, O_WRONLY | O_NONBLOCK)) != -1) {  /* mplayer running */
					write(fd, "vo_fullscreen 0\n", 17);
					close(fd);
				}
				if ((cpid = fork()) == -1)
					exit(2);
				if (cpid == 0)
					execl("/bin/sh", "sh", "-c",
						"printf \"\n\n\n\" | dmenu -c -l 3 -p \"Battery low!\"", (char *) NULL);
			}
			str += sprintf(str, "Bat:%d  ", bat);
		}

		/* Network */
		if ((fp = fopen(NET_FILE, "r"))) {
			fscanf(fp, "%4s", net);
			fclose(fp);
			str += sprintf(str, "Net:%s  ", net);
		}

		/* Time */
		time(&current);
		str += strftime(str, 14, "%b-%d  %H:%M", localtime(&current));

	XStoreName(dpy, DefaultRootWindow(dpy), status);
	XSync(dpy, False);
	}

	XCloseDisplay(dpy);
	snd_hctl_close(hctl);
	return 0;
}
