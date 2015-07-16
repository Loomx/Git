/* dstatus
 * based on one by Trilby White
 * GPL Licence
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <X11/Xlib.h>

#define TRACK_FILE    "/tmp/status_msg"
#define MEM_FILE      "/proc/meminfo"
#define VOL_FILE      "/dev/dsp"
#define BAT_NOW       "/sys/class/power_supply/BAT0/energy_now"
#define BAT_FULL      "/sys/class/power_supply/BAT0/energy_full"

#define TRACK_STR     "%s | "
#define MEM_STR       "Mem:%ld | "
#define VOL_STR       "Vol:%d | "
#define BAT_STR       "Bat:%ld | "
#define TIME_STR      "%H:%M"

int
main(void) {
	Display *dpy;
	int fd, num;
	long lnum1, lnum2, lnum3, lnum4;
	char track[50], statnext[50], status[100];
	time_t current;
	FILE *fp;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "ERROR: could not open display\n");
		exit(1);
	}

	for (;;sleep(1)) {
		status[0]='\0';

		/* Track */
		if ((fp = fopen(TRACK_FILE, "r"))) {
			//fgets(track, sizeof(track), fp);
			//track[strlen(track)-1] = ' ';
			fscanf(fp, "%48[^.\n]", track);
			fclose(fp);
			sprintf(statnext, TRACK_STR, track);
			strcat(status, statnext);
		}

		/* Memory */
		if ((fp = fopen(MEM_FILE, "r"))) {
			fscanf(fp, "MemTotal: %ld kB\nMemFree: %ld kB\nBuffers: %ld kB\nCached: %ld kB\n",
			           &lnum1, &lnum2, &lnum3, &lnum4);
			fclose(fp);
			sprintf(statnext, MEM_STR, (lnum1-(lnum2+lnum3+lnum4))/(lnum1/100));
			strcat(status, statnext);
		}

		/* Volume */
		if ((fd = open(VOL_FILE, O_WRONLY))) {
			ioctl(fd, SOUND_MIXER_READ_VOLUME, &num);
			close(fd);
			sprintf(statnext, VOL_STR, num/256);
			strcat(status, statnext);
		}

		/* Battery */
		if ((fp = fopen(BAT_NOW, "r"))) {
			fscanf(fp, "%ld\n", &lnum1);
			fclose(fp);
			fp = fopen(BAT_FULL, "r");
			fscanf(fp, "%ld\n", &lnum2);
			fclose(fp);
			sprintf(statnext, BAT_STR, (lnum1/(lnum2/100)));
			strcat(status, statnext);
		}

		/* Time */
		time(&current);
		strftime(statnext, 38, TIME_STR, localtime(&current));
		strcat(status, statnext);

	XStoreName(dpy, DefaultRootWindow(dpy), status);
	XSync(dpy, False);
	}

	XCloseDisplay(dpy);
	return 0;
}
