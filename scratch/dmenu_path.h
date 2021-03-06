#include <dirent.h>
#include <sys/stat.h>

#define CACHE  ".dmenu_cache"

static int qstrcmp(const void *a, const void *b);
static void die(const char *s);
static void scan(void);
static int uptodate(void);

static char **items = NULL;
static const char *HOME, *PATH;
static size_t count = 0;

int
updatecache(void) {
	if(!(HOME = getenv("HOME")))
		die("no $HOME");
	if(!(PATH = getenv("PATH")))
		die("no $PATH");
	if(chdir(HOME) < 0)
		die("chdir failed");
	if(uptodate() == 0)
		scan();
	return EXIT_SUCCESS;
}

void
die(const char *s) {
	fprintf(stderr, "dmenu_path: %s\n", s);
	exit(EXIT_FAILURE);
}

int
qstrcmp(const void *a, const void *b) {
	return strcmp(*(const char **)a, *(const char **)b);
}

void
scan(void) {
	char buf[PATH_MAX];
	char *dir, *path;
	size_t i;
	struct dirent *ent;
	DIR *dp;
	FILE *cache;

	if(!(path = strdup(PATH)))
		die("strdup failed");
	for(dir = strtok(path, ":"); dir; dir = strtok(NULL, ":")) {
		if(!(dp = opendir(dir)))
			continue;
		while((ent = readdir(dp))) {
			snprintf(buf, sizeof buf, "%s/%s", dir, ent->d_name);
			if(ent->d_name[0] == '.' || access(buf, X_OK) < 0)
				continue;
			if(!(tokens = realloc(tokens, ++count * sizeof *tokens)))
				die("malloc failed");
			if(!(tokens[count-1] = strdup(ent->d_name)))
				die("strdup failed");
		}
		closedir(dp);
	}
	qsort(tokens, count, sizeof *tokens, qstrcmp);
	if(!(cache = fopen(CACHE, "w")))
		die("open failed");
	for(i = 0; i < count; i++) {
		if(i > 0 && !strcmp(tokens[i], tokens[i-1]))
			continue;
		fprintf(cache,  "%s\n", tokens[i]);
	}
	fclose(cache);
	free(path);
}

int
uptodate(void) {
	char *dir, *path;
	time_t mtime;
	struct stat st;

	if(stat(CACHE, &st) < 0)
		return 0;
	mtime = st.st_mtime;
	if(!(path = strdup(PATH)))
		die("strdup failed");
	for(dir = strtok(path, ":"); dir; dir = strtok(NULL, ":"))
		if(!stat(dir, &st) && st.st_mtime > mtime)
			return 0;
	free(path);
	return 1;
}
