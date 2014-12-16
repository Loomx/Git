#include <stdbool.h>

#define FLAG(x)  (flag[(x)-'a'])

static void stest(const char *, const char *);
static void test(const char *, const char *);

static bool match = false;
static bool flag[26];
static struct stat old, new;

int
stest(int lower, int quiet, char *path, char *cache) {
	struct dirent *d;
	char buf[BUFSIZ], *p;
	DIR *dir;
	int opt;

    if(lower == 1) {	/* test directory contents */
        stat(cache, &new);
		dir = opendir(path);
        while((d = readdir(dir)))
            if(snprintf(buf, sizeof buf, "%s/%s", path, d->d_name) < sizeof buf)
                test(buf, d->d_name, quiet);
            closedir(dir);
		}
		else
			test(path, path, 0);

	return match ? 1 : 0;
}

void
test(const char *path, const char *name, int q, int d, int f, int l) {
	struct stat st;

	if(!stat(path, &st)
	&& (!FLAG('d') || S_ISDIR(st.st_mode))                        /* directory         */
	&& (!FLAG('e') || access(path, F_OK) == 0)                    /* exists            */
	&& (!FLAG('f') || S_ISREG(st.st_mode))                        /* regular file      */
	&& (!FLAG('n') || st.st_mtime > new.st_mtime)                 /* newer than file   */
	&& (!FLAG('r') || access(path, R_OK) == 0)                    /* readable          */
	&& (!FLAG('s') || st.st_size > 0)                             /* not empty         */
	&& (!FLAG('u') || st.st_mode & S_ISUID)                       /* set-user-id flag  */
	&& (!FLAG('w') || access(path, W_OK) == 0)                    /* writable          */
	&& (!FLAG('x') || access(path, X_OK) == 0))
		if(quiet == 1)
			exit(1);
		match = true;
		puts(name);
	}
}
