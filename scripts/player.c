#include <errno.h>
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

static void chomp(char *s);
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
	    //scan();
        printf("scanning...\n");

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
	char buf[PATH_MAX];
	char *dir, *path, **bins = NULL;
	size_t i, count = 0;
	struct dirent *ent;
	DIR *dp;
	FILE *cache;

	for(dir = strtok(path, ":"); dir; dir = strtok(NULL, ":")) {
		if(!(dp = opendir(dir)))
			continue;
		while((ent = readdir(dp))) {
			snprintf(buf, sizeof buf, "%s/%s", dir, ent->d_name);
			if(ent->d_name[0] == '.' || access(buf, X_OK) < 0)
				continue;
			if(!(bins = realloc(bins, ++count * sizeof *bins)))
				eprintf("malloc failed");
			if(!(bins[count-1] = strdup(ent->d_name)))
				eprintf("strdup failed");
		}
		closedir(dp);
	}
	qsort(bins, count, sizeof *bins, qstrcmp);
	if(!(cache = fopen(TRACKCACHE, "w")))
		eprintf("open failed");
	for(i = 0; i < count; i++) {
		if(i > 0 && !strcmp(bins[i], bins[i-1]))
			continue;
		fprintf(cache, "%s\n", bins[i]);
	}
	fclose(cache);
	free(path);
}

int
uptodate(void) {
	char *line = NULL;
    size_t len = 0;
	struct stat st;
	time_t mtime;
    FILE *cache;

    cache = fopen(ALBUMCACHE, "r");
	if(stat(ALBUMCACHE, &st) < 0)
		return 0;

	mtime = st.st_mtime;
    while((getline(&line, &len, cache)) != -1) {
        line[strlen(line) - 1] = '\0';
		if((stat(line, &st) < 0) || st.st_mtime > mtime) {
            free(line);
            fclose(cache);
			return 0;
        }
    }
    free(line);
    fclose(cache);
    return 1;
}
