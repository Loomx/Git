/* dstatus
 * based on one by Trilby White
 * GPL Licence
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <X11/Xlib.h>

#define TRACK_FILE      "/tmp/status_msg"
#define MEM_FILE        "/proc/meminfo"
#define VOL_FILE        "/tmp/volume"
#define BAT_NOW         "/sys/class/power_supply/BAT0/energy_now"
#define BAT_FULL        "/sys/class/power_supply/BAT0/energy_full"
//#define BAT_NOW         "/sys/class/power_supply/BAT0/charge_now"
//#define BAT_FULL        "/sys/class/power_supply/BAT0/charge_full"

#define TRACK_STR       "%s   "
#define MEM_STR         "Mem:%ld  "
#define VOL_STR         "Vol:%d  "
#define BAT_STR         "Bat:%ld  "
#define TIME_STR        "%H:%M"

int
main(void) {
	Display *dpy;
	int num;
	long lnum1, lnum2, lnum3, lnum4;
//	long lnum1, lnum2, lnumX, lnum3, lnum4;
	char track[50], status[100], *str;
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
			//fgets(track, sizeof(track), fp);
			//track[strlen(track)-1] = ' ';
			fscanf(fp, "%49[^.\n]", track);
			fclose(fp);
			str += sprintf(str, TRACK_STR, track);
		}

		/* Memory */
		if ((fp = fopen(MEM_FILE, "r"))) {
			fscanf(fp, "MemTotal: %ld kB\nMemFree: %ld kB\nBuffers: %ld kB\nCached: %ld kB\n",
			            &lnum1, &lnum2, &lnum3, &lnum4);
//			fscanf(fp, "MemTotal: %ld kB\nMemFree: %ld kB\nMemAvailable: %ld kB\nBuffers: %ld kB\nCached: %ld kB\n",
//			            &lnum1, &lnum2, &lnumX, &lnum3, &lnum4);
			fclose(fp);
			str += sprintf(str, MEM_STR, (lnum1-(lnum2+lnum3+lnum4))/(lnum1/100));
		}

		/* Volume */
		if ((fp = fopen(VOL_FILE, "r"))) {
			fscanf(fp, "%d", &num);
			fclose(fp);
			str += sprintf(str, VOL_STR, num);
		}

		/* Battery */
		if ((fp = fopen(BAT_NOW, "r"))) {
			fscanf(fp, "%ld\n", &lnum1);
			fclose(fp);
			fp = fopen(BAT_FULL, "r");
			fscanf(fp, "%ld\n", &lnum2);
			fclose(fp);
			str += sprintf(str, BAT_STR, (lnum1/(lnum2/100)));
		}

		/* Time */
		time(&current);
		str += strftime(str, 10, TIME_STR, localtime(&current));

	XStoreName(dpy, DefaultRootWindow(dpy), status);
	XSync(dpy, False);
	}

	XCloseDisplay(dpy);
	return 0;
}
