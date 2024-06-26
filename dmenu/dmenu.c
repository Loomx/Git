/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif
#include "draw.h"

#define INTERSECT(x,y,w,h,r)  (MAX(0, MIN((x)+(w),(r).x_org+(r).width)  - MAX((x),(r).x_org)) \
                             * MAX(0, MIN((y)+(h),(r).y_org+(r).height) - MAX((y),(r).y_org)))
#define MIN(a,b)              ((a) < (b) ? (a) : (b))
#define MAX(a,b)              ((a) > (b) ? (a) : (b))

typedef struct Item Item;
struct Item {
	char *text;
	Item *left, *right;
	int out;
};

static void appenditem(Item *item, Item **list, Item **last);
static void buttonpress(XEvent *e);
static void calcoffsets(void);
static char *cistrstr(const char *s, const char *sub);
static void drawmenu(void);
static void grabkeyboard(void);
static void insert(const char *str, ssize_t n);
static void keypress(XKeyEvent *ev);
static void match(void);
static int max_textw(void);
static size_t nextrune(int inc);
static void paste(void);
static int qstrcmp(const void *a, const void *b);
static void readstdin(void);
static void run(void);
static void scan(void);
static void setup(void);
static void updatecache(void);
static int uptodate(void);
static void usage(void);

static char text[BUFSIZ] = "";
static int bh, mw, mh;
static int inputw, promptw;
static size_t cursor = 0;
static unsigned long normcol[ColLast];
static unsigned long selcol[ColLast];
static unsigned long outcol[ColLast];
static Atom clip, utf8;
static DC *dc;
static Item *items = NULL;
static Item *matches, *matchend;
static Item *prev, *curr, *next, *sel;
static const char *HOME, *PATH;
static Window win;
static XIC xic;
static int mon = -1;
static int dmenurun = 0;

#include "config.h"

static int (*fstrncmp)(const char *, const char *, size_t) = strncmp;
static char *(*fstrstr)(const char *, const char *) = strstr;

int
main(int argc, char *argv[]) {
	int i, fast = 0;

    if (!strcmp(argv[0], "dmenu_run")) {
        dmenurun = 1;
		fast = 1;
        updatecache();
    }

	for (i = 1; i < argc; i++)
		/* these options take no arguments */
		if (!strcmp(argv[i], "-v")) {      /* prints version information */
			puts("dmenu-"VERSION", © 2006-2014 dmenu engineers, see LICENSE for details");
			exit(EXIT_SUCCESS);
		}
		else if (!strcmp(argv[i], "-b"))   /* appears at the bottom of the screen */
			topbar = 0;
		else if (!strcmp(argv[i], "-c"))   /* centers dmenu on screen */
			centered = 1;
		else if (!strcmp(argv[i], "-f"))   /* grabs keyboard before reading stdin */
			fast = 1;
		else if (!strcmp(argv[i], "-i")) { /* case-insensitive item matching */
			fstrncmp = strncasecmp;
			fstrstr = cistrstr;
		}
		else if (i+1 == argc)
			usage();
		/* these options take one argument */
		else if (!strcmp(argv[i], "-l"))   /* number of lines in vertical list */
			lines = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-m"))
			mon = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-p"))   /* adds prompt to left of input field */
			prompt = argv[++i];
		else if (!strcmp(argv[i], "-fn"))  /* font or font set */
			font = argv[++i];
		else if (!strcmp(argv[i], "-nb"))  /* normal background color */
			normbgcolor = argv[++i];
		else if (!strcmp(argv[i], "-nf"))  /* normal foreground color */
			normfgcolor = argv[++i];
		else if (!strcmp(argv[i], "-sb"))  /* selected background color */
			selbgcolor = argv[++i];
		else if (!strcmp(argv[i], "-sf"))  /* selected foreground color */
			selfgcolor = argv[++i];
		else
			usage();

	dc = initdc();
	initfont(dc, font);

	if (fast) {
		grabkeyboard();
		readstdin();
	}
	else {
		readstdin();
		grabkeyboard();
	}
	setup();
	run();

	return 1; /* unreachable */
}

void
appenditem(Item *item, Item **list, Item **last) {
	if (*last)
		(*last)->right = item;
	else
		*list = item;

	item->left = *last;
	item->right = NULL;
	*last = item;
}

void
buttonpress(XEvent *e) {
	int curpos;
	Item *item;
	XButtonPressedEvent *ev = &e->xbutton;

	if(ev->window != win)
		return;

	/* right-click: exit */
	if(ev->button == Button3)
		exit(EXIT_FAILURE);

	dc->x = 0;
	dc->y = 0;
	dc->h = bh;

	if(prompt && *prompt) {
		dc->w = promptw;
		dc->x = dc->w;
	}
	/* input field */
	dc->w = (lines > 0 || !matches) ? mw - dc->x : inputw;
	if((curpos = textnw(dc, text, cursor) + dc->h/2 - 2) < dc->w);

	/* left-click on input: clear input,
	 * NOTE: if there is no left-arrow the space for < is reserved so
	 *       add that to the input width */
	if(ev->button == Button1 &&
	   ((lines <= 0 && ev->x >= 0 && ev->x <= dc->x + dc->w +
	   ((!prev || !curr->left) ? textw(dc, "<") : 0)) ||
	   (lines > 0 && ev->y >= dc->y && ev->y <= dc->y + dc->h))) {
		insert(NULL, 0 - cursor);
		drawmenu();
		return;
	}
	/* middle-mouse click: paste selection */
	if(ev->button == Button2) {
		XConvertSelection(dc->dpy, (ev->state & ShiftMask) ? clip : XA_PRIMARY,
		                  utf8, utf8, win, CurrentTime);
		drawmenu();
		return;
	}
	/* scroll up */
	if(ev->button == Button4 && prev) {
		sel = curr = prev;
		calcoffsets();
		drawmenu();
		return;
	}
	/* scroll down */
	if(ev->button == Button5 && next) {
		sel = curr = next;
		calcoffsets();
		drawmenu();
		return;
	}
	if(ev->button != Button1)
		return;
	if(lines > 0) {
		/* vertical list: left-click on item */
		dc->w = mw - dc->x;
		for(item = curr; item != next; item = item->right) {
			dc->y += dc->h;
			if(ev->y >= dc->y && ev->y <= (dc->y + dc->h)) {
				if (dmenurun) {
					execlp(item->text, item->text, NULL);  /* dmenu_run */
					eprintf("exec failed");
				}
				puts(item->text);
				exit(EXIT_SUCCESS);
			}
		}
	}
	else if(matches) {
		/* left-click on left arrow */
		dc->x += inputw;
		dc->w = textw(dc, "<");
		if(prev && curr->left) {
			if(ev->x >= dc->x && ev->x <= dc->x + dc->w) {
				sel = curr = prev;
				calcoffsets();
				drawmenu();
				return;
			}
		}
		/* horizontal list: left-click on item */
		for(item = curr; item != next; item = item->right) {
			dc->x += dc->w;
			dc->w = MIN(textw(dc, item->text), mw - dc->x - textw(dc, ">"));
			if(ev->x >= dc->x && ev->x <= (dc->x + dc->w)) {
				if (dmenurun) {
					execlp(item->text, item->text, NULL);  /* dmenu_run */
					eprintf("exec failed");
				}
				puts(item->text);
				exit(EXIT_SUCCESS);
			}
		}
		/* left-click on right arrow */
		dc->w = textw(dc, ">");
		dc->x = mw - dc->w;
		if(next && ev->x >= dc->x && ev->x <= dc->x + dc->w) {
			sel = curr = next;
			calcoffsets();
			drawmenu();
			return;
		}
	}
}

void
calcoffsets(void) {
	int i, n;

	if (lines > 0)
		n = lines * bh;
	else
		n = mw - (promptw + inputw + textw(dc, "<") + textw(dc, ">"));
	/* calculate which items will begin the next page and previous page */
	for (i = 0, next = curr; next; next = next->right)
		if ((i += (lines > 0) ? bh : MIN(textw(dc, next->text), n)) > n)
			break;
	for (i = 0, prev = curr; prev && prev->left; prev = prev->left)
		if ((i += (lines > 0) ? bh : MIN(textw(dc, prev->left->text), n)) > n)
			break;
}

char *
cistrstr(const char *s, const char *sub) {
	size_t len;

	for (len = strlen(sub); *s; s++)
		if (!strncasecmp(s, sub, len))
			return (char *)s;
	return NULL;
}

void
drawmenu(void) {
	int curpos;
	Item *item;

	dc->x = 0;
	dc->y = 0;
	dc->h = bh;
	drawrect(dc, 0, 0, mw, mh, 1, BG(dc, normcol));

	if (prompt && *prompt) {
		dc->w = promptw;
		drawtext(dc, prompt, selcol);
		dc->x = dc->w;
	}
	/* draw input field */
	dc->w = (lines > 0 || !matches) ? mw - dc->x : inputw;
	drawtext(dc, text, normcol);
	if ((curpos = textnw(dc, text, cursor) + dc->h/2 - 2) < dc->w)
		drawrect(dc, curpos, 2, 1, dc->h - 4, 1, FG(dc, normcol));

	if (lines > 0) {
		/* draw vertical list */
		dc->w = mw - dc->x;
		for (item = curr; item != next; item = item->right) {
			dc->y += dc->h;
			drawtext(dc, item->text, (item == sel) ? selcol :
			                         (item->out)   ? outcol : normcol);
		}
	}
	else if (matches) {
		/* draw horizontal list */
		dc->x += inputw;
		dc->w = textw(dc, "<");
		if (curr->left)
			drawtext(dc, "<", normcol);
		for (item = curr; item != next; item = item->right) {
			dc->x += dc->w;
			dc->w = MIN(textw(dc, item->text), mw - dc->x - textw(dc, ">"));
			drawtext(dc, item->text, (item == sel) ? selcol :
			                         (item->out)   ? outcol : normcol);
		}
		dc->w = textw(dc, ">");
		dc->x = mw - dc->w;
		if (next)
			drawtext(dc, ">", normcol);
	}
	mapdc(dc, win, mw, mh);
}

void
grabkeyboard(void) {
	int i;

	/* try to grab keyboard, we may have to wait for another process to ungrab */
	for (i = 0; i < 1000; i++) {
		if (XGrabKeyboard(dc->dpy, DefaultRootWindow(dc->dpy), 1,
		                 GrabModeAsync, GrabModeAsync, CurrentTime) == GrabSuccess)
			return;
		usleep(1000);
	}
	eprintf("cannot grab keyboard\n");
}

void
insert(const char *str, ssize_t n) {
	if (strlen(text) + n > sizeof text - 1)
		return;
	/* move existing text out of the way, insert new text, and update cursor */
	memmove(&text[cursor + n], &text[cursor], sizeof text - cursor - MAX(n, 0));
	if (n > 0)
		memcpy(&text[cursor], str, n);
	cursor += n;
	match();
}

void
keypress(XKeyEvent *ev) {
	char buf[32];
	int len;
	KeySym ksym = NoSymbol;
	Status status;

	len = XmbLookupString(xic, ev, buf, sizeof buf, &ksym, &status);
	if (status == XBufferOverflow)
		return;
	if (ev->state & ControlMask)
		switch(ksym) {
		case XK_a: ksym = XK_Home;      break;
		case XK_b: ksym = XK_Left;      break;
		case XK_c: ksym = XK_Escape;    break;
		case XK_d: ksym = XK_Delete;    break;
		case XK_e: ksym = XK_End;       break;
		case XK_f: ksym = XK_Right;     break;
		case XK_g: ksym = XK_Escape;    break;
		case XK_h: ksym = XK_BackSpace; break;
		case XK_i: ksym = XK_Tab;       break;
		case XK_j: /* fallthrough */
		case XK_J: /* fallthrough */
		case XK_m: /* fallthrough */
		case XK_M: ksym = XK_Return; ev->state &= ~ControlMask; break;
		case XK_n: ksym = XK_Down;      break;
		case XK_p: ksym = XK_Up;        break;

		case XK_k: /* delete right */
			text[cursor] = '\0';
			match();
			break;
		case XK_u: /* delete left */
			insert(NULL, 0 - cursor);
			break;
		case XK_w: /* delete word */
			while (cursor > 0 && text[nextrune(-1)] == ' ')
				insert(NULL, nextrune(-1) - cursor);
			while (cursor > 0 && text[nextrune(-1)] != ' ')
				insert(NULL, nextrune(-1) - cursor);
			break;
		case XK_y: /* paste selection */
		case XK_Y:
			XConvertSelection(dc->dpy, (ev->state & ShiftMask) ? clip : XA_PRIMARY,
			                  utf8, utf8, win, CurrentTime);
			return;
		case XK_Return:
		case XK_KP_Enter:
			break;
		case XK_bracketleft:
			exit(EXIT_FAILURE);
		default:
			return;
		}
	else if (ev->state & Mod1Mask)
		switch(ksym) {
		case XK_g: ksym = XK_Home;  break;
		case XK_G: ksym = XK_End;   break;
		case XK_h: ksym = XK_Left;  break;
		case XK_j: ksym = XK_Down;  break;
		case XK_k: ksym = XK_Up;    break;
		case XK_l: ksym = XK_Right; break;
		default:
			return;
		}
	switch(ksym) {
	default:
		if (!iscntrl(*buf))
			insert(buf, len);
		break;
	case XK_Delete:
		if (text[cursor] == '\0')
			return;
		cursor = nextrune(+1);
		/* fallthrough */
	case XK_BackSpace:
		if (cursor == 0)
			return;
		insert(NULL, nextrune(-1) - cursor);
		break;
	case XK_End:
		if (text[cursor] != '\0') {
			cursor = strlen(text);
			break;
		}
		if (next) {
			/* jump to end of list and position items in reverse */
			curr = matchend;
			calcoffsets();
			curr = prev;
			calcoffsets();
			while (next && (curr = curr->right))
				calcoffsets();
		}
		sel = matchend;
		break;
	case XK_Escape:
		exit(EXIT_FAILURE);
	case XK_Home:
		if (sel == matches) {
			cursor = 0;
			break;
		}
		sel = curr = matches;
		calcoffsets();
		break;
	case XK_Left:
		if (cursor > 0 && (!sel || !sel->left || lines > 0)) {
			cursor = nextrune(-1);
			break;
		}
		if (lines > 0)
			return;
		/* fallthrough */
	case XK_Up:
		if (sel && sel->left && (sel = sel->left)->right == curr) {
			curr = prev;
			calcoffsets();
		}
		break;
	case XK_Next:
		if (!next)
			return;
		sel = curr = next;
		calcoffsets();
		break;
	case XK_Prior:
		if (!prev)
			return;
		sel = curr = prev;
		calcoffsets();
		break;
	case XK_Return:
	case XK_KP_Enter:
        if (dmenurun) {
            execlp(sel->text, sel->text, NULL);  /* dmenu_run */
            eprintf("exec failed");
		}
		puts((sel && !(ev->state & ShiftMask)) ? sel->text : text);
		if (!(ev->state & ControlMask))
			exit(EXIT_SUCCESS);
		if (sel)
			sel->out = 1;
		break;
	case XK_Right:
		if (text[cursor] != '\0') {
			cursor = nextrune(+1);
			break;
		}
		if (lines > 0)
			return;
		/* fallthrough */
	case XK_Down:
		if (sel && sel->right && (sel = sel->right) == next) {
			curr = next;
			calcoffsets();
		}
		break;
	case XK_Tab:
		if (!sel)
			return;
		strncpy(text, sel->text, sizeof text - 1);
		text[sizeof text - 1] = '\0';
		cursor = strlen(text);
		match();
		break;
	}
	drawmenu();
}

void
match(void) {
	static char **tokv = NULL;
	static int tokn = 0;

	char buf[sizeof text], *s;
	int i, tokc = 0;
	size_t len;
	Item *item, *lprefix, *lsubstr, *prefixend, *substrend;

	strcpy(buf, text);
	/* separate input text into tokens to be matched individually */
	for (s = strtok(buf, " "); s; tokv[tokc-1] = s, s = strtok(NULL, " "))
		if (++tokc > tokn && !(tokv = realloc(tokv, ++tokn * sizeof *tokv)))
			eprintf("cannot realloc %u bytes\n", tokn * sizeof *tokv);
	len = tokc ? strlen(tokv[0]) : 0;

	matches = lprefix = lsubstr = matchend = prefixend = substrend = NULL;
	for (item = items; item && item->text; item++) {
		for (i = 0; i < tokc; i++)
			if (!fstrstr(item->text, tokv[i]))
				break;
		if (i != tokc) /* not all tokens match */
			continue;
		/* exact matches go first, then prefixes, then substrings */
		if (!tokc || !fstrncmp(tokv[0], item->text, len+1))
			appenditem(item, &matches, &matchend);
		else if (!fstrncmp(tokv[0], item->text, len))
			appenditem(item, &lprefix, &prefixend);
		else
			appenditem(item, &lsubstr, &substrend);
	}
	if (lprefix) {
		if (matches) {
			matchend->right = lprefix;
			lprefix->left = matchend;
		}
		else
			matches = lprefix;
		matchend = prefixend;
	}
	if (lsubstr) {
		if (matches) {
			matchend->right = lsubstr;
			lsubstr->left = matchend;
		}
		else
			matches = lsubstr;
		matchend = substrend;
	}
	curr = sel = matches;
	calcoffsets();
}

int
max_textw(void) {
	int len = 0;
	Item *item;

	for (item = items; item && item->text; item++)
		len = MAX(strlen(item->text), len);
	return len;
}

size_t
nextrune(int inc) {
	ssize_t n;

	/* return location of next utf8 rune in the given direction (+1 or -1) */
	for (n = cursor + inc; n + inc >= 0 && (text[n] & 0xc0) == 0x80; n += inc);
	return n;
}

void
paste(void) {
	char *p, *q;
	int di;
	unsigned long dl;
	Atom da;

	/* we have been given the current selection, now insert it into input */
	XGetWindowProperty(dc->dpy, win, utf8, 0, (sizeof text / 4) + 1, 0,
	                   utf8, &da, &di, &dl, &dl, (unsigned char **)&p);
	insert(p, (q = strchr(p, '\n')) ? q-p : (ssize_t)strlen(p));
	XFree(p);
	drawmenu();
}

int
qstrcmp(const void *a, const void *b) {
	return strcmp(*(const char **)a, *(const char **)b);
}

void
readstdin(void) {
	char buf[sizeof text], *p, *maxstr = NULL;
	size_t i, max = 0, size = 0;
	FILE *cache;

    if (dmenurun) {
        if (!(cache = fopen(CACHE, "r")))
            eprintf("open failed");
	}
	/* read each line from stdin or cache and add it to the item list */
	for (i = 0; fgets(buf, sizeof buf, (dmenurun) ? cache : stdin); i++) {
		if (i+1 >= size / sizeof *items)
			if (!(items = realloc(items, (size += BUFSIZ))))
				eprintf("cannot realloc %u bytes:", size);
		if ((p = strchr(buf, '\n')))
			*p = '\0';
		if (!(items[i].text = strdup(buf)))
			eprintf("cannot strdup %u bytes:", strlen(buf)+1);
		items[i].out = 0;
		if (strlen(items[i].text) > max)
			max = strlen(maxstr = items[i].text);
	}
	if (items)
		items[i].text = NULL;
	inputw = maxstr ? textw(dc, maxstr) : 0;
	lines = MIN(lines, i);

    if (dmenurun)
        fclose(cache);
}

void
run(void) {
	XEvent ev;

	while (!XNextEvent(dc->dpy, &ev)) {
		if (XFilterEvent(&ev, win))
			continue;
		switch(ev.type) {
		case ButtonPress:
			buttonpress(&ev);
			break;
		case Expose:
			if (ev.xexpose.count == 0)
				mapdc(dc, win, mw, mh);
			break;
		case KeyPress:
			keypress(&ev.xkey);
			break;
		case SelectionNotify:
			if (ev.xselection.property == utf8)
				paste();
			break;
		case VisibilityNotify:
			if (ev.xvisibility.state != VisibilityUnobscured)
				XRaiseWindow(dc->dpy, win);
			break;
		}
	}
}

void
scan(void) {
	char buf[PATH_MAX];
	char *dir, *path, **bins = NULL;
	size_t i, count = 0;
	struct dirent *ent;
	DIR *dp;
	FILE *cache;

	if (!(path = strdup(PATH)))
		eprintf("strdup failed");
	for (dir = strtok(path, ":"); dir; dir = strtok(NULL, ":")) {
		if (!(dp = opendir(dir)))
			continue;
		while ((ent = readdir(dp))) {
			snprintf(buf, sizeof buf, "%s/%s", dir, ent->d_name);
			if (ent->d_name[0] == '.' || access(buf, X_OK) < 0)
				continue;
			if (!(bins = realloc(bins, ++count * sizeof *bins)))
				eprintf("malloc failed");
			if (!(bins[count-1] = strdup(ent->d_name)))
				eprintf("strdup failed");
		}
		closedir(dp);
	}
	qsort(bins, count, sizeof *bins, qstrcmp);
	if (!(cache = fopen(CACHE, "w")))
		eprintf("open failed");
	for (i = 0; i < count; i++) {
		if (i > 0 && !strcmp(bins[i], bins[i-1]))
			continue;
		fprintf(cache, "%s\n", bins[i]);
	}
	fclose(cache);
	free(path);
}

void
setup(void) {
	int x, y, screen = DefaultScreen(dc->dpy);
	Window root = RootWindow(dc->dpy, screen);
	XSetWindowAttributes swa;
	XIM xim;
#ifdef XINERAMA
	int n;
	XineramaScreenInfo *info;
#endif

	normcol[ColBG] = getcolor(dc, normbgcolor);
	normcol[ColFG] = getcolor(dc, normfgcolor);
	selcol[ColBG]  = getcolor(dc, selbgcolor);
	selcol[ColFG]  = getcolor(dc, selfgcolor);
	outcol[ColBG]  = getcolor(dc, outbgcolor);
	outcol[ColFG]  = getcolor(dc, outfgcolor);

	clip = XInternAtom(dc->dpy, "CLIPBOARD",   0);
	utf8 = XInternAtom(dc->dpy, "UTF8_STRING", 0);

	/* calculate menu geometry */
	bh = dc->font.height + 2;
	lines = MAX(lines, 0);
	mh = (lines + 1) * bh;
	promptw = (prompt && *prompt) ? textw(dc, prompt) : 0;
#ifdef XINERAMA
	if ((info = XineramaQueryScreens(dc->dpy, &n))) {
		int a, j, di, i = 0, area = 0;
		unsigned int du;
		Window w, pw, dw, *dws;
		XWindowAttributes wa;

		XGetInputFocus(dc->dpy, &w, &di);
		if (mon != -1 && mon < n)
			i = mon;
		if (!i && w != root && w != PointerRoot && w != None) {
			/* find top-level window containing current input focus */
			do {
				if (XQueryTree(dc->dpy, (pw = w), &dw, &w, &dws, &du) && dws)
					XFree(dws);
			} while (w != root && w != pw);
			/* find xinerama screen with which the window intersects most */
			if (XGetWindowAttributes(dc->dpy, pw, &wa))
				for (j = 0; j < n; j++)
					if ((a = INTERSECT(wa.x, wa.y, wa.width, wa.height, info[j])) > area) {
						area = a;
						i = j;
					}
		}
		/* no focused window is on screen, so use pointer location instead */
		if (mon == -1 && !area && XQueryPointer(dc->dpy, root, &dw, &dw, &x, &y, &di, &di, &du))
			for (i = 0; i < n; i++)
				if (INTERSECT(x, y, 1, 1, info[i]) != 0)
					break;

		if (centered) {
			mw = MIN(MAX(max_textw() + promptw, min_width), info[i].width);
			x = info[i].x_org + ((info[i].width  - mw) / 2);
			y = info[i].y_org + ((info[i].height - mh) / 2);
		} else {
			x = info[i].x_org;
			y = info[i].y_org + (topbar ? 0 : info[i].height - mh);
			mw = info[i].width;
		}
		XFree(info);
	}
	else
#endif
	{
		if (centered) {
			mw = MIN(MAX(max_textw() + promptw, min_width), DisplayWidth(dc->dpy, screen));
			x = (DisplayWidth(dc->dpy, screen)  - mw) / 2;
			y = (DisplayHeight(dc->dpy, screen) - mh) / 2;
		} else {
			x = 0;
			y = topbar ? 0 : DisplayHeight(dc->dpy, screen) - mh;
			mw = DisplayWidth(dc->dpy, screen);
		}
	}
	inputw = MIN(inputw, mw/3);
	match();

	/* create menu window */
	swa.override_redirect = 1;
	swa.background_pixel = normcol[ColBG];
	swa.event_mask = ExposureMask | KeyPressMask | VisibilityChangeMask |
	                 ButtonPressMask;
	win = XCreateWindow(dc->dpy, root, x, y, mw, mh, 0,
	                    DefaultDepth(dc->dpy, screen), CopyFromParent,
	                    DefaultVisual(dc->dpy, screen),
	                    CWOverrideRedirect | CWBackPixel | CWEventMask, &swa);

	/* open input methods */
	xim = XOpenIM(dc->dpy, NULL, NULL, NULL);
	xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
	                XNClientWindow, win, XNFocusWindow, win, NULL);

	XMapRaised(dc->dpy, win);
	resizedc(dc, mw, mh);
	drawmenu();
}

void
updatecache(void) {
	if (!(HOME = getenv("HOME")))
		eprintf("no $HOME");
	if (!(PATH = getenv("PATH")))
		eprintf("no $PATH");
	if (chdir(HOME) < 0)
		eprintf("chdir failed");
	if (!(uptodate()))
	    scan();
}

int
uptodate(void) {
	char *dir, *path;
	time_t mtime;
	struct stat st;

	if (stat(CACHE, &st) < 0)
		return 0;
	mtime = st.st_mtime;
	if (!(path = strdup(PATH)))
		eprintf("strdup failed");
	for (dir = strtok(path, ":"); dir; dir = strtok(NULL, ":"))
		if (!stat(dir, &st) && st.st_mtime > mtime)
			return 0;
	free(path);
	return 1;
}

void
usage(void) {
	fputs("usage: dmenu [-b] [-c] [-f] [-i] [-l lines] [-p prompt] [-fn font] [-m monitor]\n"
	      "             [-nb color] [-nf color] [-sb color] [-sf color] [-v]\n"
	      "       dmenu_run\n", stderr);
	exit(EXIT_FAILURE);
}
