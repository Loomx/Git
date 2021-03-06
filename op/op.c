/* op
 * a simple file opener in C
 * GPL Licence
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define PDF_OPEN "atril"
#define PIC_OPEN "gthumb"
#define DOC_OPEN "soffice"
#define DEFAULT  "mplayer"

int
main(int argc, char *argv[])
{
	char *suffix;

	if (argc < 2)
		return 1;

	freopen("/dev/null", "w", stderr);

	if (!(suffix = strrchr(argv[1], '.')))
		execlp("setsid", "setsid", DEFAULT, argv[1], NULL);

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
	      || !strcasecmp(suffix, ".xlsx"))
		execlp("setsid", "setsid", DOC_OPEN, argv[1], NULL);

	else
		execlp("setsid", "setsid", DEFAULT, argv[1], NULL);
}
