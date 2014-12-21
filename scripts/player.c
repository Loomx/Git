//#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define ALBUMCACHE ".mplayer/album_cache"
#define TRACKCACHE ".mplayer/track_cache"
#define MUSICDIR   "Music"
#define MPOUTPUT   "/tmp/mp_output"
#define STATUSMSG  "/tmp/status_msg"

static void eprintf(const char *s);
static int qstrcmp(const void *a, const void *b);
static void scan(void);
static int uptodate(void);

static const char *HOME;

int
main(void) {

	/* Check cache files and update if needed */
	if(!(HOME = getenv("HOME")))
		eprintf("no $HOME");
	if(chdir(HOME) < 0)
		eprintf("chdir failed");

	if(!uptodate())
	{
		scan();
		printf("scanning...\n");
	}

	/* Open dmenu to choose an album */

	/* Open dmenu to prompt for filters | trackname */

	/* Start mplayer with tracklist */

	/* Set up loop whle mplayer is running to update track name for dstatus */

	/* Clean up when mplayer exits */

}

void
eprintf(const char *s) {
	fprintf(stderr, "player: %s\n", s);
	exit(EXIT_FAILURE);
}

int
qstrcmp(const void *a, const void *b) {
	return strcmp(*(const char **)a, *(const char **)b);
}

void
scan(void) {
	char **album = NULL;
	char buf[PATH_MAX];
	char path[PATH_MAX];
	size_t i, count = 0;
	struct dirent *ent;
	FILE *cache, *cache2;
	DIR *dp;

	dp = opendir(MUSICDIR);
	while((ent = readdir(dp))) {
		if(ent->d_name[0] == '.')
			continue;
		if(!(album = realloc(album, ++count * sizeof *album)))
			eprintf("malloc failed");
		if(!(album[count-1] = strdup(ent->d_name)))
			eprintf("strdup failed");
	}
	closedir(dp);

	qsort(album, count, sizeof *album, qstrcmp);
	if(!(cache = fopen(ALBUMCACHE, "w")))
		eprintf("cache open failed");
	if(!(cache2 = fopen(TRACKCACHE, "w")))
		eprintf("cache2 open failed");
	for(i = 0; i < count; i++) {
		if(i > 0 && !strcmp(album[i], album[i-1]))
			continue;
		fprintf(cache, "%s\n", album[i]);

		path[0] = '\0';
		snprintf(path, sizeof path, "%s/%s/%s", HOME, MUSICDIR, album[i]);
		dp = opendir(path);
		while((ent = readdir(dp))) {
			if(ent->d_name[0] == '.')
				continue;
			snprintf(buf, sizeof buf, "%s/%s", path, ent->d_name);
			fprintf(cache2, "%s\n", buf);
		}
		closedir(dp);
	}
	fclose(cache);
	fclose(cache2);
}

int
uptodate(void) {
	char path[PATH_MAX];
	struct dirent *ent;
	struct stat st;
	time_t mtime;
	DIR *dp;

	if(stat(ALBUMCACHE, &st) < 0)
		return 0;
	mtime = st.st_mtime;

	if(stat(MUSICDIR, &st) < 0)
		eprintf("opening MUSICDIR failed");
	if(st.st_mtime > mtime)
		return 0;

	dp = opendir(MUSICDIR);
	while((ent = readdir(dp))) {
		if(ent->d_name[0] == '.')
			continue;
		snprintf(path, sizeof path, "%s/%s", MUSICDIR, ent->d_name);
		stat(path, &st);
		if(st.st_mtime > mtime) {
			closedir(dp);
			return 0;
		}
	}
	closedir(dp);

	if(stat(TRACKCACHE, &st) < 0)
		return 0;

	return 1;
}
