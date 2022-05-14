/* /use/local/bin/xidle                             */
/*                                                  */
/* Compile with:  gcc -o xidle xidle.c -lXss -lX11  */

#include <stdio.h>
#include <X11/extensions/scrnsaver.h>

int main(void) {
	Display *dpy = XOpenDisplay(NULL);

	if (!dpy) {
		return(1);
	}

	XScreenSaverInfo *info = XScreenSaverAllocInfo();
	XScreenSaverQueryInfo(dpy, DefaultRootWindow(dpy), info);
	printf("%lu\n", info->idle / 1000);

	XFree(info);
	XCloseDisplay(dpy);
	return(0);
}
