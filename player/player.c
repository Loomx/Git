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
#define PLAYLIST   "/tmp/playlist"
#define PLAYER     "mplayer"
#define DMENU      "dmenu"
//#define MPOUTPUT   "/tmp/mp_output"
//#define STATUSMSG  "/tmp/status_msg"

static char *dmenu(const int m, const char *dir);
static void die(const char *s);
static int qstrcmp(const void *a, const void *b);
static void scan(void);
static void setup(void);
static int uptodate(void);

const char *HOME;

int
main(int argc, char *argv[])
{
	int fd, len;
	char args[16];
	const char *album, *trackname;
	//const char *album, *filters, *trackname;

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
			die("args too long");
		write(fd, args, strlen(args));
		close(fd);
		exit(EXIT_SUCCESS);
	}
	if (argc > 1)
		exit(EXIT_SUCCESS);

	/* Check cache files and update if needed */
	setup();
	if (!uptodate())
		scan();

	/* Open dmenu to choose an album */
	album = dmenu(0, NULL);

	/* Open dmenu to prompt for filters or trackname */
	if (!strcmp(album, "Jukebox")) {
		//TODO: filters
		//filters = dmenu(1, NULL);
		execlp(PLAYER, PLAYER, "-shuffle", "-playlist", TRACKCACHE, NULL);
		die("exec PLAYER failed");
	}
	else if (!strcmp(album, "DVD")) {
		execlp(PLAYER, PLAYER, "dvd://", NULL);
		die("exec PLAYER failed");
	}
	else if (album[0] != '\0') {
		if (chdir(album) < 0)
			die("chdir $album failed");
		printf("\n");
		trackname = dmenu(2, album);
		if (!strcmp(trackname, "Play")) {
			execlp(PLAYER, PLAYER, "-playlist", PLAYLIST, NULL);
			die("exec PLAYER failed");
		}
		else if (!strcmp(trackname, "Shuffle")) {
			execlp(PLAYER, PLAYER, "-shuffle", "-playlist", PLAYLIST, NULL);
			die("exec PLAYER failed");
		}
		else {
			execlp(PLAYER, PLAYER, trackname, NULL);
			die("exec PLAYER failed");
		}
	}
	else
		exit(EXIT_SUCCESS);  /* fall through */

	/* Set up loop whle mplayer is running to update track name for dstatus */

	/* Clean up when mplayer exits */

	exit(EXIT_SUCCESS);
}

void
die(const char *s)
{
	fprintf(stderr, "player: %s\n", s);
	exit(EXIT_FAILURE);
}

char *
dmenu(const int m, const char *dir)
{
	char line[PATH_MAX];
	char **tracklist = NULL;
	int i, count = 0;
	int pipe1[2], pipe2[2], pipe3[2];
	pid_t cpid;
	size_t nread;
	static char list[PATH_MAX], sel[PATH_MAX];
	struct dirent *ent;
	DIR *dp;
	FILE *fp;

	if (pipe(pipe1) == -1 || pipe(pipe2) == -1 || pipe(pipe3) == -1 )
		die("pipe failed");
	cpid = fork();
	if (cpid == -1)
		die("fork failed");

	if (cpid == 0) {  /* child */
		cpid = fork();
		if (cpid == -1)
			die("inner fork failed");
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
					die("fopen failed");
				while ((nread = fread(line, 1, PATH_MAX, fp)) > 0)
					write(1, line, nread);
				fclose(fp);
				_exit(EXIT_SUCCESS);
			}
			else if (m == 2) {
				printf("Play\nShuffle\n");

				if (!(dp = opendir(".")))
					die("opendir $album failed");
				while ((ent = readdir(dp))) {
					if (ent->d_name[0] == '.')
						continue;
					if (!(tracklist  = realloc(tracklist, ++count * sizeof *tracklist)))
						die("malloc failed");
					if (!(tracklist[count-1] = strdup(ent->d_name)))
						die("strdup failed");
				}
				closedir(dp);

				qsort(tracklist, count, sizeof *tracklist, qstrcmp);
				if ((fp = fopen(PLAYLIST, "w")) == NULL)
					die("fopen failed");
				for(i = 0; i < count; i++) {
					fprintf(fp, "%s/%s/%s/%s\n", HOME, MUSICDIR, dir, tracklist[i]);
					snprintf(line, sizeof line, "%s\n", tracklist[i]);
					strcat(list, line);
					}
				printf("%s", list);
				fclose(fp);
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
		if (m == 1)
			execlp(DMENU, DMENU, "-p", "Filters?", NULL);
		else
			execlp(DMENU, DMENU, "-i", "-l", "40", NULL);
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

int
qstrcmp(const void *a, const void *b)
{
	return strcmp(*(const char **)a, *(const char **)b);
}

void
scan(void)
{
	char **dir = NULL;
	char path[PATH_MAX];
	int i, count = 0;
	struct dirent *ent;
	DIR *dp;
	FILE *cache, *cache2;

	if (!(dp = opendir(".")))
		die("opendir $MUSICDIR failed");
	while ((ent = readdir(dp))) {
		if (ent->d_name[0] == '.')
			continue;
		if (!(dir = realloc(dir, ++count * sizeof *dir)))
			die("malloc failed");
		if (!(dir[count-1] = strdup(ent->d_name)))
			die("strdup failed");
	}
	closedir(dp);

	qsort(dir, count, sizeof *dir, qstrcmp);
	if (!(cache = fopen(ALBUMCACHE, "w")))
		die("fopen failed");
	if (!(cache2 = fopen(TRACKCACHE, "w")))
		die("fopen2 failed");
	fprintf(cache, "Jukebox\nDVD\n");
	for(i = 0; i < count; i++) {
		if (i > 0 && !strcmp(dir[i], dir[i-1]))
			continue;
		fprintf(cache, "%s\n", dir[i]);

		if (!(dp = opendir(dir[i])))
			die("opendir $album failed");
		while ((ent = readdir(dp))) {
			if (ent->d_name[0] == '.')
				continue;
			path[0] = '\0';
			//snprintf(path, sizeof path, "%s/%s/%s/%s", HOME, MUSICDIR, dir[i], ent->d_name); /* full paths */
			snprintf(path, sizeof path, "%s/%s", dir[i], ent->d_name);
			fprintf(cache2, "%s\n", path);
		}
		closedir(dp);
	}
	fclose(cache);
	fclose(cache2);
}

void
setup(void)
{
	if (!(HOME = getenv("HOME")))
		die("no $HOME");
	if (chdir(HOME) < 0)
		die("chdir $HOME failed");
	if (chdir(MUSICDIR) < 0)
		die("chdir $MUSICDIR failed");
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
		die("opendir $MUSICDIR failed");
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
