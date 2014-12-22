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
	if(chdir(MUSICDIR) < 0)
		eprintf("$MUSICDIR failed");

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
	char path[PATH_MAX];
	size_t i, count = 0;
	struct dirent *ent;
	FILE *cache, *cache2;
	DIR *dp;

	dp = opendir(".");
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
		eprintf("fopen failed");
	if(!(cache2 = fopen(TRACKCACHE, "w")))
		eprintf("fopen2 failed");
	for(i = 0; i < count; i++) {
		if(i > 0 && !strcmp(album[i], album[i-1]))
			continue;
		fprintf(cache, "%s\n", album[i]);

		dp = opendir(album[i]);
		while((ent = readdir(dp))) {
			if(ent->d_name[0] == '.')
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
uptodate(void) {
	struct dirent *ent;
	struct stat st;
	time_t mtime;
	DIR *dp;

	if(stat(ALBUMCACHE, &st) < 0)
		return 0;
	mtime = st.st_mtime;

	if(stat(TRACKCACHE, &st) < 0)
		return 0;

	dp = opendir(".");
	while((ent = readdir(dp))) {
		if(!strcmp(ent->d_name, ".."))
			continue;
		stat(ent->d_name, &st);
		if(st.st_mtime > mtime) {
			closedir(dp);
			return 0;
		}
	}
	closedir(dp);
	return 1;
}
