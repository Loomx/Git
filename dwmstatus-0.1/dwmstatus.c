/* Jonny's dwmstatus
 *
 * Compile with:
 * gcc -Wall -pedantic -std=c99 -lX11 dwmstatus.c -o dwmstatus
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <X11/Xlib.h>

/* Files read for system info: */
#define TRACK_FILE      "/tmp/mp_track"
#define MEM_FILE        "/proc/meminfo"
#define VOL_FILE        "/tmp/alsa_volume"
#define BATT_NOW        "/sys/class/power_supply/BAT0/energy_now"
#define BATT_FULL       "/sys/class/power_supply/BAT0/energy_full"

/* Display format strings: */
#define TRACK_STR       "%s | "
#define MEM_STR         "%ldMB | "
#define VOL_STR         "%ddB | "
#define BATT_STR        "±%ld%% | "
#define TIME_STR        "%H:%M"

int
main(void) {
    Display *dpy;
    int num;
    long lnum1, lnum2, lnum3, lnum4;
    char track[30], statnext[30], status[100];
    time_t current;
    FILE *fp;

    /* Setup X display */
    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "ERROR: could not open display\n");
        exit(1);
    }

/* MAIN LOOP STARTS HERE: */
    for (;;sleep(1)) {
        status[0]='\0';

    /* Track */
        if ((fp = fopen(TRACK_FILE, "r"))) {
            //fgets(track, sizeof(track), fp);
            //track[strlen(track)-1] = ' ';
            fscanf(fp, "%28[^.]", track);
            fclose(fp);
            sprintf(statnext, TRACK_STR, track);
            strcat(status, statnext);
        }

    /* Memory */
        if ((fp = fopen(MEM_FILE, "r"))) {
            fscanf(fp, "MemTotal: %ld kB\nMemFree: %ld kB\nBuffers: %ld kB\nCached: %ld kB\n", &lnum1, &lnum2, &lnum3, &lnum4);
            fclose(fp);
            sprintf(statnext, MEM_STR, (lnum1-(lnum2+lnum3+lnum4))/1024);
            strcat(status, statnext);
        }

    /* Volume */
        if ((fp = fopen(VOL_FILE, "r"))) {
            fscanf(fp, "%d", &num);
            fclose(fp);
            sprintf(statnext, VOL_STR, num);
            strcat(status, statnext);
        }

    /* Battery */
        if ((fp = fopen(BATT_NOW, "r"))) {
            fscanf(fp, "%ld\n", &lnum1);
            fclose(fp);
            fp = fopen(BATT_FULL, "r");
            fscanf(fp, "%ld\n", &lnum2);
            fclose(fp);
            sprintf(statnext, BATT_STR, (lnum1/(lnum2/100)));
            strcat(status, statnext);
        }

    /* Time */
        time(&current);
        strftime(statnext, 38, TIME_STR, localtime(&current));
        strcat(status, statnext);

    /* Set root name */
    XStoreName(dpy, DefaultRootWindow(dpy), status);
    XSync(dpy, False);
    }

/* NEXT LINES SHOULD NEVER EXECUTE */
    XCloseDisplay(dpy);
    return 0;
}
