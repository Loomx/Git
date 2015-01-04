//#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define MUSICDIR   "Music"
#define FIFO       "/home/jonny/.mplayer/mp_pipe" 
//#define FIFO       ".mp_pipe" 
#define ALBUMCACHE ".album_cache"
#define TRACKCACHE ".track_cache"
#define PLAYER     "mplayer"
#define DMENU      "dmenu"
#define MPOUTPUT   "/tmp/mp_output"
#define STATUSMSG  "/tmp/status_msg"

static char *dmenu(const int m);
static void eprintf(const char *s);
static int qstrcmp(const void *a, const void *b);
static void scan(void);
static void setup(void);
static int uptodate(void);

int
main(int argc, char *argv[])
{
	int fd, len, mode;
	char *album;
	char args[16];

	/* Check for arguments and send to PLAYER */
	mknod(FIFO, S_IFIFO | 0644, 0);
	if ((fd = open(FIFO, O_WRONLY | O_NONBLOCK)) != -1) {  /* PLAYER running */
		if (argc > 2)
			len = snprintf(args, sizeof args, "%s %s\n", argv[1], argv[2]);
		else if (argc == 2)
			len = snprintf(args, sizeof args, "%s\n", argv[1]);
		else
			len = snprintf(args, sizeof args, "pause\n");
		if (len >= sizeof args)
			eprintf("args too long");
		write(fd, args, strlen(args));
		close(fd);
		exit(EXIT_SUCCESS);
	}
	if (argc > 1)
		exit(EXIT_SUCCESS);

	/* Check cache files and update if needed */
	setup();
	if (!uptodate()) {
		scan();
		printf("scanning...\n");
	}

	/* Open dmenu to choose an album */
	album = dmenu(0);
	if (album[0] == '\0')
		exit(EXIT_SUCCESS);
	else if (!strcmp(album, "DVD"))
		execlp(PLAYER, PLAYER, "dvd://", NULL);
	else if (!strcmp(album, "Jukebox"))
		mode = 1;
	else mode = 2;

	//printf("Selection = %s\nMode = %d\n", album, mode);

	/* Open dmenu to prompt for filters (mode == 1) or trackname (mode ==2) */

	/* Start mplayer with tracklist */
	if (mode == 1)
		execlp(PLAYER, PLAYER, "-shuffle", "-playlist", TRACKCACHE, NULL);

	/* Set up loop whle mplayer is running to update track name for dstatus */

	/* Clean up when mplayer exits */

	printf("exiting at end of main()\n");
	exit(EXIT_SUCCESS);
}

void
setup(void)
{
	const char *HOME;

	if (!(HOME = getenv("HOME")))
		eprintf("no $HOME");
	if (chdir(HOME) < 0)
		eprintf("chdir $HOME failed");
	if (chdir(MUSICDIR) < 0)
		eprintf("chdir $MUSICDIR failed");
}

char *
dmenu(const int m)
{
	char line[PATH_MAX];
	static char sel[PATH_MAX];
	int pipe1[2], pipe2[2], pipe3[2];
	pid_t cpid;
	size_t nread;
	FILE *fp;

	if (pipe(pipe1) == -1 || pipe(pipe2) == -1 || pipe(pipe3) == -1 )
		eprintf("pipe failed");
	cpid = fork();
	if (cpid == -1)
		eprintf("fork failed");

	if (cpid == 0) {  /* child */
		cpid = fork();
		if (cpid == -1)
			eprintf("inner fork failed");
		if (cpid == 0) {  /* grandchild */
			close(pipe1[0]);  /* unused */
			close(pipe2[1]);  /* unused */
			close(pipe3[0]);  /* unused */
			close(pipe3[1]);  /* unused */
			dup2(pipe2[0], 0);
			close(pipe2[0]);  /* dup2ed */
			dup2(pipe1[1], 1);
			close(pipe1[1]);  /* dup2ed */
			if (m == 0) {
				if ((fp = fopen(ALBUMCACHE, "r")) == NULL)
					eprintf("fopen failed\n");
				while ((nread = fread(line, 1, PATH_MAX, fp)) > 0)
					write(1, line, nread);
				fclose(fp);
			_exit(EXIT_SUCCESS);
			}
		} else {  /* child */
		close(pipe1[1]);  /* unused */
		close(pipe2[0]);  /* unused */
		close(pipe2[1]);  /* unused */
		close(pipe3[0]);  /* unused */
		dup2(pipe1[0], 0);
		close(pipe1[0]);  /* dup2ed */
		dup2(pipe3[1], 1);
		close(pipe3[1]);  /* dup2ed */
		if (m == 0)
			execlp(DMENU, DMENU, "-i", "-l", "40", NULL);
		else
			execlp(DMENU, DMENU, "-p", "Filters?", NULL);
		}

	} else {  /* parent */
		close(pipe1[0]);  /* unused */
		close(pipe1[1]);  /* unused */
		close(pipe2[0]);  /* unused */
		close(pipe2[1]);  /* unused */
		close(pipe3[1]);  /* unused */
		if ((nread = read(pipe3[0], sel, PATH_MAX)) > 0)
			sel[nread - 1] = '\0';
		close(pipe3[0]);
	}
	return sel;
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
		if (!strcmp(ent->d_name, "..") || !strcmp(ent->d_name, FIFO))
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
