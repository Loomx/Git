/* dstatus
 * based on one by Trilby White
 * GPL Licence
 *
 * nolibc version
 *
 * compile with: 
gcc -fno-asynchronous-unwind-tables -fno-ident -s -Os -nostdlib -static -lgcc \
	-I path/to/nolibc -o dstatus dstatus-nolibc.c
 */

#include "nolibc.h"

#define TRACK_FILE      "/tmp/status_msg"
#define MEM_FILE        "/proc/meminfo"
#define VOL_FILE        "/tmp/volume"
#define BAT_FILE        "/sys/class/power_supply/BAT0/capacity"
#define FIFO            "/tmp/mp_pipe"
#define NET_FILE        "/sys/class/net/wlan0/operstate"

#define LOW_BAT_LVL     8

int
main(void) {
//	Display *dpy;
	int bat, cpid, fd, i, len, reset;
	long lnum1, lnum2, lnum3, lnum4;
	char buf[5], track[64], scratch[156], status[128], *ptr, *str;
	time_t current;

//	if (!(dpy = XOpenDisplay(NULL))) {
//		fprintf(stderr, "ERROR: could not open display\n");
//		exit(1);
//	}

	for (;;sleep(1)) {
		str = status;

		/* Track */
		if ((fd = open(TRACK_FILE, O_RDONLY)) != -1) {
			len = read(fd, track, 64);
			track[len - 1] = '\0';
			close(fd);
			str += strlcpy(str, track, 64);
		}

		/* Memory */
		if ((fd = open(MEM_FILE, O_RDONLY)) != -1) {
			len = read(fd, scratch, 156);
			for (i = 16; scratch[i] == ' '; i++);
			lnum1 = atol(scratch + i);
			for (i = 44; scratch[i] == ' '; i++);
			lnum2 = atol(scratch + i);
			for (i = 100; scratch[i] == ' '; i++);
			lnum3 = atol(scratch + i);
			for (i = 128; scratch[i] == ' '; i++);
			lnum4 = atol(scratch + i);
			ptr = ltoa((lnum1-(lnum2+lnum3+lnum4))/(lnum1/100));
			str += strlcpy(str, "    Mem:", 8);
			str += strlcpy(str, ptr, 8);
			close(fd);
		}

		/* Volume */
		if ((fd = open(VOL_FILE, O_RDONLY)) != -1) {
			len = read(fd, buf, 4);
			buf[len - 1] = '\0';
			close(fd);
			str += strlcpy(str, "  Vol:", 8);
			str += strlcpy(str, buf, 4);
		}

		/* Battery */
		if ((fd = open(BAT_FILE, O_RDONLY)) != -1) {
			len = read(fd, buf, 4);
			buf[len - 1] = '\0';
			close(fd);
			bat = atoi(buf);
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
				if (cpid == 0) {
					char *arr[] = { "sh", "-c", "dmenu -c -l 3 -p 'Battery low!' <<<$'\n\n'", NULL };
					execve("/bin/sh", arr, NULL);
				}
			}
			str += strlcpy(str, "  Bat:", 8);
			str += strlcpy(str, buf, 5);
		}

		/* Network */
		if ((fd = open(NET_FILE, O_RDONLY)) != -1) {
			len = read(fd, buf, 5);
			buf[len - 1] = '\0';
			close(fd);
			str += strlcpy(str, "  Net:", 8);
			str += strlcpy(str, buf, 5);
		}

		/* Time */
		current = time(NULL);
		//lnum1 = current % 60;  // secs
		current /= 60;
		lnum2 = current % 60;  // minutes
		current /= 60;
		current += 13;  /* summer = 13, winter = 12 */
		lnum3 = current % 24;  // hours
		str += strlcpy(str, "  ", 2);
		str += strlcpy(str, ltoa(lnum3), 2);
		str += strlcpy(str, ":", 1);
		if (lnum2 < 10)
			str += strlcpy(str, "0", 1);
		str += strlcpy(str, ltoa(lnum2), 2);

//		XStoreName(dpy, DefaultRootWindow(dpy), status);
//		XSync(dpy, False);
		printf("%s\n", status);
	}

//	XCloseDisplay(dpy);
	return 0;
}
