#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define MUSICDIR   "Music"
#define FIFO       ".mp_pipe"
#define ALBUMCACHE ".album_cache"
#define TRACKCACHE ".track_cache"
#define PLAYLIST   "/tmp/playlist"
#define STATUSMSG  "/tmp/status_msg"

static void die(const char *s);
static char *dmenu(const int m);
static void filter(void);
static void mplayer(const int m);
static int qstrcmp(const void *a, const void *b);
static void scan(void);
static void setup(void);
static int uptodate(void);

static char *filters;
static const char *album, *trackname, *HOME;

int
main(int argc, char *argv[])
{
	int fd, len;
	char args[80];

	/* Check for arguments and send to mplayer via FIFO */
	setup();
	mknod(FIFO, S_IFIFO | 0644, 0);
	if ((fd = open(FIFO, O_WRONLY | O_NONBLOCK)) != -1) {  /* mplayer running */
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
	if (!uptodate())
		scan();

	/* Choose an album */
	album = dmenu(0);

	/* Prompt for filters | mode | trackname, then launch mplayer */
	if (!strcmp(album, "Jukebox")) {
		filters = dmenu(1);
		if (!*filters) {
			mplayer(0); /* shuffle all */
		}
		else {
			filter();
			mplayer(1); /* shuffle playlist */
		}
	}
	else if (!strcmp(album, "DVD")) {
		mplayer(2); /* play dvd */
	}
	else if (*album) {
		if (chdir(album) < 0)
			die("chdir $album failed");
		printf("\n");
		trackname = dmenu(2);
		if (!strcmp(trackname, "Play"))
			mplayer(3); /* play playlist */

		else if (!strcmp(trackname, "Shuffle"))
			mplayer(1); /* shuffle playlist */

		else
			mplayer(4); /* play track */
	}

	/* Clean up after mplayer exits */
	unlink(PLAYLIST);
	unlink(STATUSMSG);
	exit(EXIT_SUCCESS);
}

void
die(const char *s)
{
	fprintf(stderr, "player: %s\n", s);
	exit(EXIT_FAILURE);
}

char *
dmenu(const int m)
{
	char line[PATH_MAX];
	char **tracklist = NULL;
	int i, count = 0;
	int pipe1[2], pipe2[2], pipe3[2];
	pid_t cpid;
	size_t nread;
	static char sel[NAME_MAX];
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
			else if (m == 1) {
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
					printf("%s\n", tracklist[i]);
					fprintf(fp, "%s/%s/%s/%s\n", HOME, MUSICDIR, album, tracklist[i]);
					}
				fclose(fp);
				_exit(EXIT_SUCCESS);
			}
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
		if ((nread = read(pipe3[0], sel, PATH_MAX)) > 0)
			sel[nread - 1] = '\0';
		close(pipe3[0]);
	}
	return sel;
}

void
filter(void)
{
	int i;
	char buf[PATH_MAX], line[PATH_MAX], lline[PATH_MAX], *s;
	FILE *fp, *fp2;

	for (i=0; filters[i]; ++i)
		filters[i] = tolower(filters[i]);
	if ((fp = fopen(TRACKCACHE, "r")) == NULL)
		die("fopen failed");
	if ((fp2 = fopen(PLAYLIST, "w")) == NULL)
		die("fopen2 failed");
	while (fgets(line, sizeof line, fp) != NULL) {
		for (i=0; line[i]; ++i)
			lline[i] = tolower(line[i]);
		lline[strlen(line)] = '\0';
		strcpy(buf, filters);
		for (s = strtok(buf, " "); s; s = strtok(NULL, " "))
			if (strstr(lline, s) != NULL)
				fprintf(fp2, "%s/%s/%s", HOME, MUSICDIR, line);
	}
	fclose(fp);
	fclose(fp2);
}

void
mplayer(const int m)
{
	char link[PATH_MAX], proc[NAME_MAX], *track;
	int len;
	pid_t cpid;
	FILE *fp;

	if (m == 2) {
		execlp("mplayer", "mplayer", "dvd://", NULL);
		die("exec mplayer failed");
	}

	cpid = fork();
	if (cpid == -1)
		die("fork failed");

	if (cpid == 0) {  /* child */
		switch(m) {
			case 0:
				execlp("mplayer", "mplayer", "-shuffle", "-playlist", TRACKCACHE, NULL);
				break;
			case 1:
				execlp("mplayer", "mplayer", "-shuffle", "-playlist", PLAYLIST, NULL);
				break;
			case 3:
				execlp("mplayer", "mplayer", "-playlist", PLAYLIST, NULL);
				break;
			case 4:
				execlp("mplayer", "mplayer", trackname, NULL);
				break;
		}
		die("exec mplayer failed");
	}
	else {  /* parent */
		sleep(1);
		sprintf(proc, "/proc/%d/fd", cpid);
		if (chdir(proc) < 0)
			die("chdir /proc/*/fd failed");
		while (readlink("3", link, sizeof link) != -1)
			while ((len = readlink("4", link, sizeof link)) > 1) {
				link[len] = '\0';
				if ((fp = fopen(STATUSMSG, "w")) == NULL)
					die("fopen failed");
				track = strrchr(link, '/');
				fprintf(fp, "%s\n", ++track);
				fclose(fp);
				sleep(1); /* TODO: use inotify here... */
			}
	}
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
		fprintf(cache, "%s\n", dir[i]);

		if (!(dp = opendir(dir[i])))
			die("opendir $dir failed");
		while ((ent = readdir(dp))) {
			if (ent->d_name[0] == '.')
				continue;
			fprintf(cache2, "%s/%s\n", dir[i], ent->d_name);		}
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
