/* player
 * a simple music player in C using dmenu and mplayer.
 * GPL Licence
 */

#define _GNU_SOURCE

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MUSICDIR   "Music"
#define ALBUMCACHE ".album_cache"
#define TRACKCACHE ".track_cache"
#define FIFO       "/tmp/mp_pipe"
#define PLAYLIST   "/tmp/playlist"
#define STATUSMSG  "/tmp/status_msg"
#define PATH_MAX   4096
#define NAME_MAX   255

static void die(const char *s);
static char *dmenu(const int m);
static void dmenuinput(const int m);
static void filter(void);
static void gettrackname(const pid_t cpid);
static pid_t mplayer(const int m);
static int qstrcmp(const void *a, const void *b);
static void scan(void);
static void setup(void);
static int uptodate(void);

static const char *album, *filters, *trackname, *HOME;

int
main(int argc, char *argv[])
{
	int fd;
	pid_t cpid;

	/* Check for arguments and send to mplayer via FIFO */
	mkfifo(FIFO, 0644);
	if ((fd = open(FIFO, O_WRONLY | O_NONBLOCK)) != -1) {  /* mplayer running */
		if (argc > 2)
			dprintf(fd, "%s %s\n", argv[1], argv[2]);
		else if (argc == 2)
			dprintf(fd, "%s\n", argv[1]);
		else
			dprintf(fd, "pause\n");
		close(fd);
		return 0;
	}
	if (argc > 1)
		return 0;

	/* Check cache files and update if needed */
	setup();
	if (!uptodate())
		scan();

	/* Choose an album */
	album = dmenu(0);

	/* Prompt for filters | mode | trackname, launch mplayer */
	if (!strcmp(album, "Jukebox")) {
		filters = dmenu(1);
		if (!*filters) {
			cpid = mplayer(0);  /* shuffle all */
		}
		else {
			filter();
			cpid = mplayer(1);  /* shuffle playlist */
		}
	}
	else if (!strcmp(album, "DVD")) {
		mplayer(2);  /* play dvd or die */
	}
	else if (*album) {
		if (chdir(album) < 0)
			die("chdir $album failed");
		trackname = dmenu(2);
		if (!strcmp(trackname, "Play"))
			cpid = mplayer(3);  /* play playlist */

		else if (!strcmp(trackname, "Shuffle"))
			cpid = mplayer(1);  /* shuffle playlist */

		else
			cpid = mplayer(4);  /* play track */
	}
	else {
		return 0;  /* nothing selected */
	}

	/* Loop while playing to save current trackname */
	gettrackname(cpid);

	/* Clean up after mplayer exits */
	unlink(PLAYLIST);
	unlink(STATUSMSG);
	return 0;
}

void
die(const char *s)
{
	fprintf(stderr, "player: %s\n", s);
	exit(1);
}

char *
dmenu(const int m)
{
	int pipe1[2], pipe2[2], pipe3[2];
	pid_t cpid;
	size_t nread;
	static char sel[NAME_MAX];

	if (pipe(pipe1) == -1 || pipe(pipe2) == -1 || pipe(pipe3) == -1)
		die("pipe failed");
	if ((cpid = fork()) == -1)
		die("fork failed");

	if (cpid == 0) {  /* child */
		if ((cpid = fork()) == -1)
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
			dmenuinput(m);
			_exit(0);
		}
		else {  /* child */
			close(pipe1[1]);  /* unused */
			close(pipe2[0]);  /* unused */
			close(pipe2[1]);  /* unused */
			close(pipe3[0]);  /* unused */
			dup2(pipe1[0], 0);
			close(pipe1[0]);  /* dup2ed */
			dup2(pipe3[1], 1);
			close(pipe3[1]);  /* dup2ed */
			if (m == 1)
				execlp("dmenu", "dmenu", "-p", "Filters?", NULL);
			else
				execlp("dmenu", "dmenu", "-i", "-l", "40", NULL);
		}
	}
	else {  /* parent */
		close(pipe1[0]);  /* unused */
		close(pipe1[1]);  /* unused */
		close(pipe2[0]);  /* unused */
		close(pipe2[1]);  /* unused */
		close(pipe3[1]);  /* unused */
		if ((nread = read(pipe3[0], sel, sizeof(sel))) > 0)
			sel[nread - 1] = '\0';
		close(pipe3[0]);
	}
	wait(NULL);
	return sel;
}

void
dmenuinput(const int m)
{
	char line[PATH_MAX];
	char **tracklist = NULL;
	int i, count = 0;
	size_t nread;
	struct dirent *ent;
	DIR *dp;
	FILE *fp;

	if (m == 0) {
		if (!(fp = fopen(ALBUMCACHE, "r")))
			die("fopen failed");
		while ((nread = fread(line, 1, PATH_MAX, fp)) > 0)
			write(1, line, nread);
		fclose(fp);
	}
	else if (m == 2) {
		write(1, "Play\nShuffle\n", 13);
		if (!(dp = opendir(".")))
			die("opendir $album failed");
		while ((ent = readdir(dp))) {
			if (ent->d_name[0] == '.')
				continue;
			if (!(tracklist = realloc(tracklist, sizeof(*tracklist) * ++count)))
				die("malloc failed");
			if (!(tracklist[count-1] = strdup(ent->d_name)))
				die("strdup failed");
		}
		closedir(dp);

		qsort(tracklist, count, sizeof(*tracklist), qstrcmp);
		if (!(fp = fopen(PLAYLIST, "w")))
			die("fopen failed");
		for (i = 0; i < count; i++) {
			write(1, tracklist[i], strlen(tracklist[i]));
			write(1, "\n", 1);
			fprintf(fp, "%s/%s/%s/%s\n", HOME, MUSICDIR, album, tracklist[i]);
			free(tracklist[i]);
		}
		fclose(fp);
		free(tracklist);
	}
}

void
filter(void)
{
	char buf[PATH_MAX], line[PATH_MAX], *s;
	FILE *fp, *fp2;

	if (!(fp = fopen(TRACKCACHE, "r")))
		die("fopen failed");
	if (!(fp2 = fopen(PLAYLIST, "w")))
		die("fopen2 failed");
	while (fgets(line, sizeof(line), fp)) {
		strcpy(buf, filters);
		for (s = strtok(buf, " "); s; s = strtok(NULL, " "))
			if (strcasestr(line, s))
				fprintf(fp2, "%s/%s/%s", HOME, MUSICDIR, line);
	}
	fclose(fp);
	fclose(fp2);
}

void
gettrackname(const pid_t cpid)
{
	char junk[PATH_MAX], link[PATH_MAX], proc[NAME_MAX], *suffix, *track;
	int len;
	FILE *fp;

	read(0, junk, sizeof(junk));
	sprintf(proc, "/proc/%d/fd/4", cpid);
	while ((len = readlink(proc, link, sizeof(link)-1)) > 1) {
		link[len] = '\0';
		track = strrchr(link, '/');
		if ((suffix = strrchr(track, '.')))
			*suffix = '\0';
		if (!(fp = fopen(STATUSMSG, "w")))
			die("fopen failed");
		fprintf(fp, "%s\n", ++track);
		fclose(fp);
		read(0, junk, sizeof(junk));
	}
}

pid_t
mplayer(const int m)
{
	int pipe1[2];
	pid_t cpid;

	if (m == 2) {
		execlp("mplayer", "mplayer", "dvd://", NULL);
		die("exec mplayer failed");
	}

	if (pipe(pipe1) == -1)
		die("pipe failed");
	if ((cpid = fork()) == -1)
		die("fork failed");

	if (cpid == 0) {  /* child */
		close(pipe1[0]);  /* unused */
		dup2(pipe1[1], 1);
		close(pipe1[1]);  /* dup2ed */
		switch (m) {
		case 0:
			execlp("mplayer", "mplayer", "-vo", "null", "-identify",
			       "-shuffle", "-playlist", TRACKCACHE, NULL);
			break;
		case 1:
			execlp("mplayer", "mplayer", "-vo", "null", "-identify",
			       "-shuffle", "-playlist", PLAYLIST, NULL);
			break;
		case 3:
			execlp("mplayer", "mplayer", "-vo", "null", "-identify",
			       "-playlist", PLAYLIST, NULL);
			break;
		case 4:
			execlp("mplayer", "mplayer", "-vo", "null", "-identify",
			       trackname, NULL);
			break;
		}
		die("exec mplayer failed");
	}
	else {  /* parent */
		close(pipe1[1]);  /* unused */
		dup2(pipe1[0], 0);
		close(pipe1[0]);  /* dup2ed */
	}
	return cpid;
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
	int i, count = 0;
	struct dirent *ent;
	DIR *dp;
	FILE *cache, *cache2;

	if (!(dp = opendir(".")))
		die("opendir $MUSICDIR failed");
	while ((ent = readdir(dp))) {
		if (ent->d_name[0] == '.')
			continue;
		if (!(dir = realloc(dir, sizeof(*dir) * ++count)))
			die("malloc failed");
		if (!(dir[count-1] = strdup(ent->d_name)))
			die("strdup failed");
	}
	closedir(dp);

	qsort(dir, count, sizeof(*dir), qstrcmp);
	if (!(cache = fopen(ALBUMCACHE, "w")))
		die("fopen failed");
	if (!(cache2 = fopen(TRACKCACHE, "w")))
		die("fopen2 failed");
	if (!(access("/dev/dvd", F_OK)))
		fprintf(cache, "Jukebox\nDVD\n");
	else
		fprintf(cache, "Jukebox\n");
	for (i = 0; i < count; i++) {
		fprintf(cache, "%s\n", dir[i]);

		if (!(dp = opendir(dir[i])))
			die("opendir $dir failed");
		while ((ent = readdir(dp))) {
			if (ent->d_name[0] == '.')
				continue;
			fprintf(cache2, "%s/%s\n", dir[i], ent->d_name);
		}
		closedir(dp);
		free(dir[i]);
	}
	fclose(cache);
	fclose(cache2);
	free(dir);
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
