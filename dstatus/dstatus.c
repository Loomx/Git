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
#include <X11/Xlib.h>

#define TRACK_FILE      "/tmp/status_msg"
#define MEM_FILE        "/proc/meminfo"
#define VOL_FILE        "/tmp/volume"
#define BAT_NOW         "/sys/class/power_supply/BAT0/charge_now"
#define BAT_FULL        "/sys/class/power_supply/BAT0/charge_full"
#define FIFO            "/tmp/mp_pipe"
#define NET_FILE        "/sys/class/net/wlan0/operstate"

#define LOW_BAT_LVL     5
#define NOTIFIER        "xmessage"
#define MESSAGE         "Battery low!"

#define TRACK_STR       "%s   "
#define MEM_STR         "Mem:%ld  "
#define VOL_STR         "Vol:%s  "
#define BAT_STR         "Bat:%d  "
#define NET_STR         "Net:%s  "
#define TIME_STR        "%b-%d  %H:%M"

int
main(void) {
	Display *dpy;
	int bat, cpid, fd, reset;
	long lnum1, lnum2, lnum3, lnum4;
	char vol[5], net[5], track[50], status[100], *str;
	time_t current;
	FILE *fp;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "ERROR: could not open display\n");
		exit(1);
	}

	for (;;sleep(1)) {
		str = status;

		/* Track */
		if ((fp = fopen(TRACK_FILE, "r"))) {
			fgets(track, 49, fp);
			track[strcspn(track, "\n")] = 0;
			fclose(fp);
			str += sprintf(str, TRACK_STR, track);
		}

		/* Memory */
		if ((fp = fopen(MEM_FILE, "r"))) {
			fscanf(fp, "MemTotal: %ld kB\nMemFree: %ld kB\nMemAvailable: %*d kB\nBuffers: %ld kB\nCached: %ld kB\n",
			            &lnum1, &lnum2, &lnum3, &lnum4);
			fclose(fp);
			str += sprintf(str, MEM_STR, (lnum1-(lnum2+lnum3+lnum4))/(lnum1/100));
		}

		/* Volume */
		if ((fp = fopen(VOL_FILE, "r"))) {
			fgets(vol, 4, fp);
			vol[strcspn(vol, "%\n")] = 0;
			fclose(fp);
			str += sprintf(str, VOL_STR, vol);
		}

		/* Battery */
		if ((fp = fopen(BAT_NOW, "r"))) {
			fscanf(fp, "%ld\n", &lnum1);
			fclose(fp);
			fp = fopen(BAT_FULL, "r");
			fscanf(fp, "%ld\n", &lnum2);
			fclose(fp);
			bat = lnum1/(lnum2/100);
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
					execlp(NOTIFIER, NOTIFIER, MESSAGE, NULL);
			}
			str += sprintf(str, BAT_STR, bat);
		}

		/* Network */
		if ((fp = fopen(NET_FILE, "r"))) {
			fscanf(fp, "%4s", net);
			fclose(fp);
			str += sprintf(str, NET_STR, net);
		}

		/* Time */
		time(&current);
		str += strftime(str, 14, TIME_STR, localtime(&current));

	XStoreName(dpy, DefaultRootWindow(dpy), status);
	XSync(dpy, False);
	}

	XCloseDisplay(dpy);
	return 0;
}
