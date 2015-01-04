/* unneeded functions from player.c */

static int albumsel(void);
static int checkplayer(void);

main()
{
	int mpid, mode;

	mpid = checkplayer();
	if (mpid == 1) {
		printf("PLAYER already running\n");
		exit(EXIT_SUCCESS);
	}

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

int
checkplayer(void)
{
	char comm[32], path[PATH_MAX];
	int nread;
	struct dirent *ent;
	DIR *dp;
	FILE *fp;

	if (!(dp = opendir("/proc")))
		eprintf("opendir /proc failed");
	while ((ent = readdir(dp))) {
		if (ent->d_name[0] != '0' &&
			ent->d_name[0] != '1' &&
			ent->d_name[0] != '2' &&
			ent->d_name[0] != '3' &&
			ent->d_name[0] != '4' &&
			ent->d_name[0] != '5' &&
			ent->d_name[0] != '6' &&
			ent->d_name[0] != '7' &&
			ent->d_name[0] != '8' &&
			ent->d_name[0] != '9')
			continue;
		snprintf(path, sizeof path, "/proc/%s/comm", ent->d_name);
		if ((fp = fopen(path, "r")) == NULL)
			continue;
		if ((nread = fread(comm, 1, 30, fp)) > 0) {
			comm[nread - 1] = '\0';
			if (!strcmp(comm, PLAYER))
				return 1;
			}
		fclose(fp);
	}
	closedir(dp);	
	return 0;

}
