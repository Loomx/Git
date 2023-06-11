/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <curses.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <locale.h>
#include <regex.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#undef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define LEN(x) (sizeof(x) / sizeof(*(x)))
#define ISODD(x) ((x) & 1)
#define CONTROL(c) ((c) ^ 0x40)
#define META(c) ((c) ^ 0x80)

struct cpair {
	int fg;
	int bg;
};

/* Supported actions */
enum action {
	SEL_QUIT = 1,
	SEL_BACK,
	SEL_GOIN,
	SEL_FLTR,
	SEL_NEXT,
	SEL_PREV,
	SEL_PGDN,
	SEL_PGUP,
	SEL_HOME,
	SEL_END,
	SEL_CD,
	SEL_CDHOME,
	SEL_TOGGLEDOT,
	SEL_DSORT,
	SEL_MTIME,
	SEL_ICASE,
	SEL_REDRAW,
	SEL_RUN,
	SEL_RUNARG
};

struct key {
	int sym;         /* Key pressed */
	enum action act; /* Action */
	char *run;       /* Program to run */
	char *env;       /* Environment variable override */
};

#include "config.h"

struct entry {
	char name[PATH_MAX];
	mode_t mode;
	time_t t;
};

/* Global context */
WINDOW *Box0, *Box1, *Text0, *Text1;
struct entry *dents, *pdents;
char *argv0;
int ndents, npdents, cur;

/*
 * Layout:
 * .-------------------.  .-------------------.
 * | /mnt/path         |  | file or dir       |
 * |                   |  |   preview here    |
 * |    file0          |  |                   |
 * |    file1          |  |                   |
 * |  > file2          |  |                   |
 * |    file3          |  |                   |
 * |    file4          |  |                   |
 * |    ...            |  |                   |
 * |    filen          |  |                   |
 * |                   |  |                   |
 * | Permission denied |  |                   |
 * '-------------------'  '-------------------'
 */

/* strlcat from nolibc */
size_t
strlcat(char *dst, const char *src, size_t size)
{
	size_t len;
	char c;

	for (len = 0; dst[len];	len++)
		;

	for (;;) {
		c = *src;
		if (len < size)
			dst[len] = c;
		if (!c)
			break;
		len++;
		src++;
	}

	return len;
}

/* strlcpy from nolibc */
size_t
strlcpy(char *dst, const char *src, size_t size)
{
	size_t len;
	char c;

	for (len = 0;;) {
		c = src[len];
		if (len < size)
			dst[len] = c;
		if (!c)
			break;
		len++;
	}
	return len;
}

void
initcurses(void)
{
	char *term;
	int i;

	if (initscr() == NULL) {
		term = getenv("TERM");
		if (term != NULL)
			fprintf(stderr, "error opening terminal: %s\n", term);
		else
			fprintf(stderr, "failed to initialize curses\n");
		exit(1);
	}
	if (usecolor && has_colors()) {
		start_color();
		use_default_colors();
		for (i = 1; i < LEN(pairs); i++)
			init_pair(i, pairs[i].fg, pairs[i].bg);
	}
	cbreak();
	noecho();
	nonl();
	intrflush(0, FALSE);
	curs_set(FALSE); /* Hide cursor */
	timeout(1000); /* One second */
}

/* Messages show up at the bottom */
void
info(char *msg)
{
	mvwprintw(Text0, LINES - 3, 1, "%s\n", msg);
	wrefresh(Text0);
}

/* Display warning as a message */
void
warn(char *msg)
{
	wclear(Text1);
	wprintw(Text1, "  %s: %s\n", msg, strerror(errno));
	wrefresh(Text1);
}

/* Kill curses and display error before exiting */
void
fatal(char *msg)
{
	endwin();
	fprintf(stderr, "%s: %s\n", msg, strerror(errno));
	exit(1);
}

/* Some implementations of dirname(3) may modify `path' and some
 * return a pointer inside `path'. */
char *
xdirname(const char *path)
{
	static char out[PATH_MAX];
	char tmp[PATH_MAX], *p;

	strlcpy(tmp, path, sizeof(tmp));
	p = dirname(tmp);
	if (p == NULL)
		fatal("dirname");
	strlcpy(out, p, sizeof(out));
	return out;
}

char *
xgetenv(char *name, char *fallback)
{
	char *value;

	if (name == NULL)
		return fallback;
	value = getenv(name);
	return value && value[0] ? value : fallback;
}

int
setfilter(regex_t *regex, char *filter)
{
	char errbuf[LINE_MAX];
	size_t len;
	int r;

	r = regcomp(regex, filter, REG_NOSUB | REG_EXTENDED | REG_ICASE);
	if (r != 0) {
		len = (COLS / 2);
		if (len > sizeof(errbuf))
			len = sizeof(errbuf);
		regerror(r, regex, errbuf, len);
		info(errbuf);
	}
	return r;
}

void
initfilter(int dot, char **ifilter)
{
	*ifilter = dot ? "." : "^[^.]";
}

int
visible(regex_t *regex, char *file)
{
	return regexec(regex, file, 0, NULL, 0) == 0;
}

int
dircmp(mode_t a, mode_t b)
{
	if (S_ISDIR(a) && S_ISDIR(b))
		return 0;
	if (!S_ISDIR(a) && !S_ISDIR(b))
		return 0;
	if (S_ISDIR(a))
		return -1;
	else
		return 1;
}

int
entrycmp(const void *va, const void *vb)
{
	const struct entry *a = va, *b = vb;

	if (dirorder) {
		if (dircmp(a->mode, b->mode) != 0)
			return dircmp(a->mode, b->mode);
	}

	if (mtimeorder)
		return b->t - a->t;
	if (icaseorder)
		return strcasecmp(a->name, b->name);
	return strcmp(a->name, b->name);
}

/* Returns SEL_* if key is bound and 0 otherwise.
 * Also modifies the run and env pointers (used on SEL_{RUN,RUNARG}) */
int
nextsel(char **run, char **env)
{
	int c, i;

	c = wgetch(Text0);
	if (c == 033)
		c = META(wgetch(Text0));

	for (i = 0; i < LEN(bindings); i++)
		if (c == bindings[i].sym) {
			*run = bindings[i].run;
			*env = bindings[i].env;
			return bindings[i].act;
		}
	return 0;
}

char *
readln(void)
{
	static char ln[LINE_MAX];

	timeout(-1);
	echo();
	curs_set(TRUE);
	memset(ln, 0, sizeof(ln));
	wgetnstr(Text0, ln, sizeof(ln) - 1);
	noecho();
	curs_set(FALSE);
	timeout(1000);
	return ln[0] ? ln : NULL;
}

int
canopendir(char *path)
{
	DIR *dirp;

	dirp = opendir(path);
	if (dirp == NULL)
		return 0;
	closedir(dirp);
	return 1;
}

char *
mkpath(char *dir, char *name, char *out, size_t n)
{
	/* Handle absolute path */
	if (name[0] == '/') {
		strlcpy(out, name, n);
	} else {
		/* Handle root case */
		if (strcmp(dir, "/") == 0) {
			strlcpy(out, "/", n);
			strlcat(out, name, n);
		} else {
			strlcpy(out, dir, n);
			strlcat(out, "/", n);
			strlcat(out, name, n);
		}
	}
	return out;
}

void
printent(struct entry *ent, int active, int win)
{
	char name[PATH_MAX];
	unsigned int len = (COLS / 2) - strlen(CURSR) - 3;
	char cm = 0;
	int attr = 0;

	/* Copy name locally */
	strlcpy(name, ent->name, sizeof(name));

	/* No text wrapping in entries */
	if (strlen(name) < len)
		len = strlen(name) + 1;

	if (S_ISDIR(ent->mode)) {
		cm = '/';
		attr |= DIR_ATTR;
	} else if (S_ISLNK(ent->mode)) {
		cm = '@';
		attr |= LINK_ATTR;
	} else if (S_ISSOCK(ent->mode)) {
		cm = '=';
		attr |= SOCK_ATTR;
	} else if (S_ISFIFO(ent->mode)) {
		cm = '|';
		attr |= FIFO_ATTR;
	} else if (ent->mode & S_IXUSR) {
		cm = '*';
		attr |= EXEC_ATTR;
	}

	if (active)
		attr |= CURSR_ATTR;

	if (cm) {
		name[len - 1] = cm;
		name[len] = '\0';
	}

	if (win == 0) {
		wattron(Text0, attr);
		wprintw(Text0, "%s%s\n", active ? CURSR : EMPTY, name);
		wattroff(Text0, attr);
	} else {
		wattron(Text1, attr);
		wprintw(Text1, "  %s\n", name);
		wattroff(Text1, attr);
	}
}

int
dentfill(char *path, struct entry **dents,
	 int (*filter)(regex_t *, char *), regex_t *re, int win)
{
	char newpath[PATH_MAX];
	DIR *dirp;
	struct dirent *dp;
	struct stat sb;
	int r, n = 0;

	dirp = opendir(path);
	if (dirp == NULL)
		return 0;

	while ((dp = readdir(dirp)) != NULL) {
		/* Skip self and parent */
		if (strcmp(dp->d_name, ".") == 0 ||
		    strcmp(dp->d_name, "..") == 0)
			continue;
		if (filter(re, dp->d_name) == 0)
			continue;
		*dents = realloc(*dents, (n + 1) * sizeof(**dents));
		if (*dents == NULL)
			fatal("realloc");
		strlcpy((*dents)[n].name, dp->d_name, sizeof((*dents)[n].name));
		/* Get mode flags */
		mkpath(path, dp->d_name, newpath, sizeof(newpath));
		r = lstat(newpath, &sb);
		if (r == -1)
			fatal("lstat");
		(*dents)[n].mode = sb.st_mode;
		(*dents)[n].t = sb.st_mtime;
		n++;
	}

	/* Should never be null */
	r = closedir(dirp);
	if (r == -1)
		fatal("closedir");
	return n;
}

void
dentfree(struct entry *dents)
{
	free(dents);
}

/* Return the position of the matching entry or 0 otherwise */
int
dentfind(struct entry *dents, int n, char *cwd, char *path)
{
	char tmp[PATH_MAX];
	int i;

	if (path == NULL)
		return 0;
	for (i = 0; i < n; i++) {
		mkpath(cwd, dents[i].name, tmp, sizeof(tmp));
		if (strcmp(tmp, path) == 0)
			return i;
	}
	return 0;
}

int
spawnlp(char *dir, char *file, char *argv0, char *argv1)
{
	pid_t pid;
	int status, r;

	pid = fork();
	switch (pid) {
	case -1:
		return -1;
	case 0:
		if (dir != NULL && chdir(dir) == -1)
			exit(1);
		execlp(file, argv0, argv1, (char *) NULL);
		_exit(1);
	default:
		while ((r = waitpid(pid, &status, 0)) == -1 && errno == EINTR)
			continue;
		if (r == -1)
			return -1;
		if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
			return -1;
	}
	return 0;
}

int
populate(char *path, char *oldpath, char *fltr)
{
	regex_t re;
	int r;

	/* Can fail when permissions change while browsing */
	if (canopendir(path) == 0)
		return -1;

	/* Search filter */
	r = setfilter(&re, fltr);
	if (r != 0)
		return -1;

	dentfree(dents);

	ndents = 0;
	dents = NULL;

	ndents = dentfill(path, &dents, visible, &re, 0);
	regfree(&re);
	if (ndents == 0)
		return 0; /* Empty result */

	qsort(dents, ndents, sizeof(*dents), entrycmp);

	/* Find cur from history */
	cur = dentfind(dents, ndents, path, oldpath);
	return 0;
}

int
ppopulate(char *path)
{
	regex_t re;

	/* Can fail when permissions change while browsing */
	if (canopendir(path) == 0)
		return -1;

	setfilter(&re, "^[^.]");

	dentfree(pdents);

	npdents = 0;
	pdents = NULL;

	npdents = dentfill(path, &pdents, visible, &re, 1);
	regfree(&re);
	if (npdents == 0)
		return 0; /* Empty result */

	qsort(pdents, npdents, sizeof(*pdents), entrycmp);

	return 0;
}

void
redraw(char *path)
{
	char cwd[PATH_MAX], cwdresolved[PATH_MAX]; 
	char newpath[PATH_MAX], line[LINE_MAX];
	struct stat sb;
	int ncols, nlines, nplines, odd;
	int i;
	int r, fd;
	FILE *fp;

	nlines = MIN(LINES - 5, ndents);

	/* Set up screen */
	Box0 = newwin(LINES, COLS / 2, 0, 0);
	box(Box0, 0, 0);
	wrefresh(Box0);

	Box1 = newwin(LINES, COLS / 2, 0, COLS / 2);
	box(Box1, 0, 0);
	wrefresh(Box1);

	Text0 = newwin(LINES - 2, (COLS / 2) - 2, 1, 1);
	keypad(Text0, TRUE);
	Text1 = newwin(LINES - 2, (COLS / 2) - 2, 1, (COLS / 2) + 1);

	/* Strip trailing slashes */
	for (i = strlen(path) - 1; i > 0; i--)
		if (path[i] == '/')
			path[i] = '\0';
		else
			break;


	/* No text wrapping in cwd line */
	ncols = (COLS / 2) - 4;
	if (ncols > PATH_MAX)
		ncols = PATH_MAX;
	strlcpy(cwd, path, ncols);
	cwd[ncols - strlen(CWD) - 1] = '\0';
	realpath(cwd, cwdresolved);

	wprintw(Text0, CWD " %s\n\n", cwdresolved);

	/* Print listing */
	if (ndents == 0)
		return;
	odd = ISODD(nlines);
	if (cur < nlines / 2) {
		for (i = 0; i < nlines; i++)
			printent(&dents[i], i == cur, 0);
	} else if (cur >= ndents - nlines / 2) {
		for (i = ndents - nlines; i < ndents; i++)
			printent(&dents[i], i == cur, 0);
	} else {
		for (i = cur - nlines / 2;
		     i < cur + nlines / 2 + odd; i++)
			printent(&dents[i], i == cur, 0);
	}
	if (ndents > LINES - 3)
		wprintw(Text0, "   %s\n", "...");
	wrefresh(Text0);

	/* Print preview */
	mkpath(path, dents[cur].name, newpath, sizeof(newpath));
	fd = open(newpath, O_RDONLY | O_NONBLOCK);
	if (fd == -1) {
		warn("open");
		return;
	}
	r = fstat(fd, &sb);
	if (r == -1) {
		warn("fstat");
		close(fd);
		return;
	}
	close(fd);

	wclear(Text1);

	switch (sb.st_mode & S_IFMT) {
	case S_IFDIR:
		if (canopendir(newpath) == 0) {
			warn("canopendir");
			return;
		}
		r = ppopulate(newpath);
		if (r == -1) {
			warn("ppopulate");
			return;
		}
		if (npdents == 0)
			return;
		nplines = MIN(LINES - 3, npdents);
		for (i = 0; i < nplines; i++)
			printent(&pdents[i], 0, 1);
		if (npdents > LINES - 3)
			wprintw(Text1, "  %s\n", "...");
		break;
		
	case S_IFREG:
		fp = fopen(newpath, "r");
		for (i = 0; i < LINES - 3; i++) {
			if (!(fgets(line, COLS / 2 - 2, fp)))
				break;
			wprintw(Text1, "  %.*s", ncols, line);
		}
		if ((fgets(line, COLS / 2 - 2, fp)))
			wprintw(Text1, "  %s\n", "...");
		fclose(fp);
		break;

	default:
		info("Unsupported file");
		break;
	}
	wrefresh(Text1);
}

void
browse(char *ipath, char *ifilter)
{
	char path[PATH_MAX], oldpath[PATH_MAX], newpath[PATH_MAX];
	char fltr[LINE_MAX];
	char *dir, *tmp, *run, *env;
	struct stat sb;
	regex_t re;
	int r, fd;

	strlcpy(path, ipath, sizeof(path));
	strlcpy(fltr, ifilter, sizeof(fltr));
	oldpath[0] = '\0';
begin:
	r = populate(path, oldpath, fltr);
	if (r == -1) {
		warn("populate");
		goto nochange;
	}

	for (;;) {
		redraw(path);
nochange:
		switch (nextsel(&run, &env)) {
		case SEL_QUIT:
			dentfree(dents);
			return;
		case SEL_BACK:
			/* There is no going back */
			if (strcmp(path, "/") == 0 ||
			    strcmp(path, ".") == 0 ||
			    strchr(path, '/') == NULL)
				goto nochange;
			dir = xdirname(path);
			if (canopendir(dir) == 0) {
				warn("canopendir");
				goto nochange;
			}
			/* Save history */
			strlcpy(oldpath, path, sizeof(oldpath));
			strlcpy(path, dir, sizeof(path));
			/* Reset filter */
			strlcpy(fltr, ifilter, sizeof(fltr));
			goto begin;
		case SEL_GOIN:
			/* Cannot descend in empty directories */
			if (ndents == 0)
				goto nochange;

			mkpath(path, dents[cur].name, newpath, sizeof(newpath));

			/* Get path info */
			fd = open(newpath, O_RDONLY | O_NONBLOCK);
			if (fd == -1) {
				warn("open");
				goto nochange;
			}
			r = fstat(fd, &sb);
			if (r == -1) {
				warn("fstat");
				close(fd);
				goto nochange;
			}
			close(fd);

			switch (sb.st_mode & S_IFMT) {
			case S_IFDIR:
				if (canopendir(newpath) == 0) {
					warn("canopendir");
					goto nochange;
				}
				strlcpy(path, newpath, sizeof(path));
				/* Reset filter */
				strlcpy(fltr, ifilter, sizeof(fltr));
				goto begin;
			case S_IFREG:
				endwin();
				run = xgetenv("OPEN", OPEN);
				r = spawnlp(path, run, run, newpath);
				initcurses();
				if (r == -1) {
					info("Failed to execute plumber");
					goto nochange;
				}
				continue;
			default:
				info("Unsupported file");
				goto nochange;
			}
		case SEL_FLTR:
			/* Read filter */
			info("/");
			tmp = readln();
			if (tmp == NULL)
				tmp = ifilter;
			/* Check and report regex errors */
			r = setfilter(&re, tmp);
			if (r != 0)
				goto nochange;
			regfree(&re);
			strlcpy(fltr, tmp, sizeof(fltr));
			/* Save current */
			if (ndents > 0)
				mkpath(path, dents[cur].name, oldpath, sizeof(oldpath));
			goto begin;
		case SEL_NEXT:
			if (cur < ndents - 1)
				cur++;
			break;
		case SEL_PREV:
			if (cur > 0)
				cur--;
			break;
		case SEL_PGDN:
			if (cur < ndents - 1)
				cur += MIN((LINES - 4) / 2, ndents - 1 - cur);
			break;
		case SEL_PGUP:
			if (cur > 0)
				cur -= MIN((LINES - 4) / 2, cur);
			break;
		case SEL_HOME:
			cur = 0;
			break;
		case SEL_END:
			cur = ndents - 1;
			break;
		case SEL_CD:
			/* Read target dir */
			info("chdir: ");
			tmp = readln();
			if (tmp == NULL) {
				info("");
				goto nochange;
			}
			mkpath(path, tmp, newpath, sizeof(newpath));
			if (canopendir(newpath) == 0) {
				warn("canopendir");
				goto nochange;
			}
			strlcpy(path, newpath, sizeof(path));
			/* Reset filter */
			strlcpy(fltr, ifilter, sizeof(fltr));
			goto begin;
		case SEL_CDHOME:
			tmp = getenv("HOME");
			if (tmp == NULL) {
				info("");
				goto nochange;
			}
			if (canopendir(tmp) == 0) {
				warn("canopendir");
				goto nochange;
			}
			strlcpy(path, tmp, sizeof(path));
			/* Reset filter */
			strlcpy(fltr, ifilter, sizeof(fltr));
			goto begin;
		case SEL_TOGGLEDOT:
			showhidden ^= 1;
			initfilter(showhidden, &ifilter);
			strlcpy(fltr, ifilter, sizeof(fltr));
			goto begin;
		case SEL_MTIME:
			mtimeorder = !mtimeorder;
			/* Save current */
			if (ndents > 0)
				mkpath(path, dents[cur].name, oldpath, sizeof(oldpath));
			goto begin;
		case SEL_DSORT:
			dirorder = !dirorder;
			/* Save current */
			if (ndents > 0)
				mkpath(path, dents[cur].name, oldpath, sizeof(oldpath));
			goto begin;
		case SEL_ICASE:
			icaseorder = !icaseorder;
			/* Save current */
			if (ndents > 0)
				mkpath(path, dents[cur].name, oldpath, sizeof(oldpath));
			goto begin;
		case SEL_REDRAW:
			/* Save current */
			if (ndents > 0)
				mkpath(path, dents[cur].name, oldpath, sizeof(oldpath));
			goto begin;
		case SEL_RUN:
			/* Save current */
			if (ndents > 0)
				mkpath(path, dents[cur].name, oldpath, sizeof(oldpath));
			run = xgetenv(env, run);
			endwin();
			spawnlp(path, run, run, (void *)0);
			initcurses();
			goto begin;
		case SEL_RUNARG:
			/* Save current */
			if (ndents > 0)
				mkpath(path, dents[cur].name, oldpath, sizeof(oldpath));
			run = xgetenv(env, run);
			endwin();
			spawnlp(path, run, run, dents[cur].name);
			initcurses();
			goto begin;
		}
	}
}

int
main(int argc, char *argv[])
{
	char cwd[PATH_MAX], *ipath;
	char *ifilter;

	/* Confirm we are in a terminal */
	if (!isatty(0) || !isatty(1)) {
		fprintf(stderr, "stdin or stdout is not a tty\n");
		exit(1);
	}

	if (getuid() == 0)
		showhidden = 1;
	initfilter(showhidden, &ifilter);

	if (argv[1] != NULL) {
		ipath = argv[1];
	} else {
		ipath = getcwd(cwd, sizeof(cwd));
		if (ipath == NULL)
			ipath = "/";
	}

	signal(SIGINT, SIG_IGN);

	/* Test initial path */
	if (canopendir(ipath) == 0) {
		fprintf(stderr, "%s: %s\n", ipath, strerror(errno));
		exit(1);
	}

	/* Set locale before curses setup */
	setlocale(LC_ALL, "");
	initcurses();
	browse(ipath, ifilter);
	endwin();
	exit(0);
}
