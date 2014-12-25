//#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define MUSICDIR   "Music"
#define ALBUMCACHE ".album_cache"
#define TRACKCACHE ".track_cache"
#define PLAYER     "/usr/bin/mplayer"
#define DMENU      "/usr/local/bin/dmenu"
#define MPOUTPUT   "/tmp/mp_output"
#define STATUSMSG  "/tmp/status_msg"

//static int albumsel(void);
static char *dmenu(const int m);
static void eprintf(const char *s);
static int qstrcmp(const void *a, const void *b);
static void scan(void);
static int uptodate(void);

static const char *HOME;

int
main(void)
{
	int mode;
	char *sel;
	//char sel[PATH_MAX];

	/* Check cache files and update if needed */
	if (!(HOME = getenv("HOME")))
		eprintf("no $HOME");
	if (chdir(HOME) < 0)
		eprintf("chdir $HOME failed");
	if (chdir(MUSICDIR) < 0)
		eprintf("chdir $MUSICDIR failed");

	if (!uptodate()) {
		scan();
		printf("scanning...\n");
	}

	/* Open dmenu to choose an album */
	sel = dmenu(1);
	if (!strcmp(sel, NULL))
		exit(EXIT_SUCCESS);
	else if (!strcmp(sel, "DVD"))
		execl(PLAYER, PLAYER, "dvd://", NULL);
	else if (!strcmp(sel, "Jukebox"))
		mode = 0;
	else mode = 1;

	printf("Selection = %s\nMode = %d\n", sel, mode);

	/* Open dmenu to prompt for filters | trackname */

	/* Start mplayer with tracklist */

	/* Set up loop whle mplayer is running to update track name for dstatus */

	/* Clean up when mplayer exits */

}

/*
int
albumsel(void)
{
	char path[PATH_MAX];
	char sel[PATH_MAX];
	int pipefd[2];
	pid_t cpid;

	if (pipe(pipefd) == -1)
		eprintf("pipe failed");
	cpid = fork();
	if (cpid == -1)
		eprintf("fork failed");

	if (cpid == 0) {
		close(pipefd[0]);

		sel = dmenu(ALBUMCACHE);

		write(pipefd[1], sel, strlen(sel));
		close(pipefd[1]);
		exit(EXIT_SUCCESS);
	} else {
		close(pipefd[1]);
		if (read(pipefd[0], &path, PATH_MAX) > 0)
			path[strlen(path)] = '\0';
		close(pipefd[0]);
		wait(NULL);
	}
	printf("%s\n", path);
}
*/

char *
dmenu(const int m)
{
	char sel[PATH_MAX];
	int out[2], ret[2];
	pid_t cpid;
	FILE *fp;

	if (pipe(out) == -1 || pipe(ret) == -1)
		eprintf("pipe failed");
	cpid = fork();
	if (cpid == -1)
		eprintf("fork failed");

	if (cpid == 0) {  /* child execs dmenu */
		close(0);
		dup(out[0]);
		close(1);
		dup(ret[1]);
		if (m == 1)
			execl(DMENU, DMENU, "-i", "-l", "40", NULL);
		else
			execl(DMENU, DMENU, "-p", "Filters?", NULL);

	} else {          /* parent */
		close(0);
		dup(ret[0]);
		close(1);
		dup(out[1]);
		if (m == 1) {
			fp = fopen(ALBUMCACHE, "r");
			while (fgets(sel, PATH_MAX, fp))
				puts(sel);
		}
		sel[0] = '\0';
		if (read(0, &sel, PATH_MAX) > 0)
			sel[strlen(sel)] = '\0';
		close(ret[0]);
		close(out[1]);
		wait(NULL);
	}
	printf("%s\n", sel);
}

void
eprintf(const char *s)
{
	fprintf(stderr, "player: %s\n", s);
	exit(EXIT_FAILURE);
}

int
qstrcmp(const void *a, const void *b)
{
	return strcmp(*(const char **)a, *(const char **)b);
}

void
scan(void)
{
	char **album = NULL;
	char path[PATH_MAX];
	size_t i, count = 0;
	struct dirent *ent;
	FILE *cache, *cache2;
	DIR *dp;

	if (!(dp = opendir(".")))
		eprintf("opendir $MUSICDIR failed");
	while ((ent = readdir(dp))) {
		if (ent->d_name[0] == '.')
			continue;
		if (!(album = realloc(album, ++count * sizeof *album)))
			eprintf("malloc failed");
		if (!(album[count-1] = strdup(ent->d_name)))
			eprintf("strdup failed");
	}
	closedir(dp);

	qsort(album, count, sizeof *album, qstrcmp);
	if (!(cache = fopen(ALBUMCACHE, "w")))
		eprintf("fopen failed");
	if (!(cache2 = fopen(TRACKCACHE, "w")))
		eprintf("fopen2 failed");
	fprintf(cache, "Jukebox\nDVD\n");
	for(i = 0; i < count; i++) {
		if (i > 0 && !strcmp(album[i], album[i-1]))
			continue;
		fprintf(cache, "%s\n", album[i]);

		if (!(dp = opendir(album[i])))
			eprintf("opendir $album failed");
		while ((ent = readdir(dp))) {
			if (ent->d_name[0] == '.')
				continue;
			path[0] = '\0';
			//snprintf(path, sizeof path, "%s/%s/%s/%s", HOME, MUSICDIR, album[i], ent->d_name); /* full paths */
			snprintf(path, sizeof path, "%s/%s", album[i], ent->d_name);
			fprintf(cache2, "%s\n", path);
		}
		closedir(dp);
	}
	fclose(cache);
	fclose(cache2);
}

int
uptodate(void)
{
	struct dirent *ent;
	struct stat st;
	time_t mtime;
	DIR *dp;

	if (stat(ALBUMCACHE, &st) < 0)
		return 0;
	mtime = st.st_mtime;

	if (stat(TRACKCACHE, &st) < 0)
		return 0;

	if (!(dp = opendir(".")))
		eprintf("opendir $MUSICDIR failed");
	while ((ent = readdir(dp))) {
		if (!strcmp(ent->d_name, ".."))
			continue;
		stat(ent->d_name, &st);
		if (st.st_mtime > mtime) {
			closedir(dp);
			return 0;
		}
	}
	closedir(dp);
	return 1;
}
