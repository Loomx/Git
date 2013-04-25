/* xystray
 * Copyright (c) 2005 Lukasz Stelmach.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
/* Sun OpenWindows */
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include "Xystray.h"

#define SYSTEM_TRAY_REQUEST_DOCK	0
#define SYSTEM_TRAY_BEGIN_MESSAGE	1
#define SYSTEM_TRAY_CANCEL_MESSAGE	2

/* Flags for _XEMBED_INFO */
#define XEMBED_MAPPED			(1 << 0)

static XrmOptionDescRec options[] = {
{"-iconmax",	"*xystray.iconmax",	XrmoptionSepArg,	NULL},
{"-iconrows",	"*xystray.iconrows",	XrmoptionSepArg,	NULL},
{"-iconcols",	"*xystray.iconcols",	XrmoptionSepArg,	NULL},
{"-iconpadding","*xystray.iconpadding",	XrmoptionSepArg,	NULL},
{"-iconsize",	"*xystray.iconsize",	XrmoptionSepArg,	NULL},

/* multiple obecnie nie jest u¿ywane/UNUSED */
{"-multiple",	"*xystray.multiple",	XrmoptionNoArg,		"TRUE"},
{"-vertical",	"*xystray.vertical",	XrmoptionNoArg,		"TRUE"},
};

static XtSignalId signalids[NSIG];

void sigwrap(int i) {
	if (signalids[i])
		XtNoticeSignal(signalids[i]);
}

void sigint(XtPointer topwidget, XtSignalId* sig) {
	Widget tw = (Widget)topwidget;
        printf("%s\n", __FUNCTION__);

	XtDestroyWidget(tw);
	XtDestroyApplicationContext(XtWidgetToApplicationContext(tw));
	exit(0);
}	

static void
quit(Widget w, XEvent *event, String *params, Cardinal *num_params) {
	XtCloseDisplay(XtDisplay(w));
	exit(0);
}

static Display *display = NULL;
/* see below 
static XtActionsRec xysact[] = {
	{ "quit", quit },
};
*/

int
main (int argc, char *argv[])
{
  XtAppContext app_con;
  Widget topwidget;
  Widget xys;
  Dimension w, h;

  memset(signalids, 0, sizeof(signalids));
  topwidget = XtOpenApplication(&app_con, "Xystray",
		options, XtNumber(options), &argc, argv,
		NULL, topLevelShellWidgetClass, NULL, 0);

  XtVaSetValues(topwidget,
	XtNallowShellResize, False,
	XtNtitle, "Xystray", NULL);

  signalids[SIGINT] = XtAppAddSignal(app_con,
			(XtSignalCallbackProc)sigint, topwidget);
  signal(SIGINT, sigwrap);

  xys = XtVaCreateManagedWidget("xystray", xystrayWidgetClass,
			topwidget, NULL);

  XtRealizeWidget(topwidget);
  display = XtDisplay(topwidget);

  XWtGetSize((XystrayWidget)xys, &w, &h);
  XtVaSetValues(topwidget,
	XtNmaxWidth, w,
	XtNminWidth, w,
	XtNmaxHeight, h,
	XtNminHeight, h,
	NULL);

  /* needs more investigation
  XtAppAddActions (app_con, xysact, XtNumber(xysact));
  XtOverrideTranslations(topwidget,    
	XtParseTranslationTable ("<Message>WM_PROTOCOLS: quit()"));*/

  XtAppMainLoop(app_con);
  return 0;
}
