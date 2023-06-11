/* op
 * a simple file opener in C
 * GPL v2 Licence
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define PDF_OPEN   "atril"
#define PIC_OPEN   "gthumb"
#define DOC_OPEN   "soffice"
#define MEDIA_OPEN "mplayer"
#define GFX_OPEN   "lightburn"
#define DEFAULT    "vi"

int
main(int argc, char *argv[])
{
	char *suffix;

	if (argc < 2)
		return 1;

	freopen("/dev/null", "w", stderr);

	if (!(suffix = strrchr(argv[1], '.'))) {
		if (!(access(argv[1], F_OK)))
			execlp(DEFAULT, DEFAULT, argv[1], NULL);
		return 0;
	}
	else if (!strcasecmp(suffix, ".pdf"))
		execlp("setsid", "setsid", PDF_OPEN, argv[1], NULL);

	else if (!strcasecmp(suffix, ".jpg")
	      || !strcasecmp(suffix, ".jpeg")
	      || !strcasecmp(suffix, ".png")
	      || !strcasecmp(suffix, ".tiff"))
		execlp("setsid", "setsid", PIC_OPEN, argv[1], NULL);

	else if (!strcasecmp(suffix, ".doc")
	      || !strcasecmp(suffix, ".docx")
	      || !strcasecmp(suffix, ".dot")
	      || !strcasecmp(suffix, ".dotx")
	      || !strcasecmp(suffix, ".ppt")
	      || !strcasecmp(suffix, ".pptx")
	      || !strcasecmp(suffix, ".xls")
	      || !strcasecmp(suffix, ".xlsx")
	      || !strcasecmp(suffix, ".odg"))
		execlp("setsid", "setsid", DOC_OPEN, argv[1], NULL);

	else if (!strcasecmp(suffix, ".mp4")
	      || !strcasecmp(suffix, ".mov")
	      || !strcasecmp(suffix, ".avi"))
		execlp("setsid", "setsid", MEDIA_OPEN, argv[1], NULL);

	else if (!strcasecmp(suffix, ".mp3")
	      || !strcasecmp(suffix, ".m4a")
	      || !strcasecmp(suffix, ".ogg"))
		execlp("setsid", "setsid", MEDIA_OPEN, "-vo", "null", argv[1], NULL);

	else if (!strcasecmp(suffix, ".lbrn")
	      || !strcasecmp(suffix, ".lbrn2")
	      || !strcasecmp(suffix, ".svg"))
		execlp("setsid", "setsid", GFX_OPEN, argv[1], NULL);

	else
		execlp(DEFAULT, DEFAULT, argv[1], NULL);
}
