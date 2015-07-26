/* op
 * a simple file opener in C
 * GPL Licence
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define PDF_OPEN "xpdf"
#define PIC_OPEN "geeqie"
#define DOC_OPEN "libreoffice"
#define DEFAULT  "mplayer"

int
main(int argc, char *argv[])
{
	char *suffix;

	suffix = strrchr(argv[1], '.');

	if (suffix == NULL)
		execlp(DEFAULT, DEFAULT, argv[1], NULL);

	if (!strcasecmp(suffix, ".pdf"))
		execlp(PDF_OPEN, PDF_OPEN, argv[1], NULL);

	else if (!strcasecmp(suffix, ".jpg"))
		execlp(PIC_OPEN, PIC_OPEN, argv[1], NULL);
	else if (!strcasecmp(suffix, ".jpeg"))
		execlp(PIC_OPEN, PIC_OPEN, argv[1], NULL);
	else if (!strcasecmp(suffix, ".tiff"))
		execlp(PIC_OPEN, PIC_OPEN, argv[1], NULL);
	else if (!strcasecmp(suffix, ".png"))
		execlp(PIC_OPEN, PIC_OPEN, argv[1], NULL);

	else if (!strcasecmp(suffix, ".doc"))
		execlp(DOC_OPEN, DOC_OPEN, argv[1], NULL);
	else if (!strcasecmp(suffix, ".docx"))
		execlp(DOC_OPEN, DOC_OPEN, argv[1], NULL);
	else if (!strcasecmp(suffix, ".ppt"))
		execlp(DOC_OPEN, DOC_OPEN, argv[1], NULL);
	else if (!strcasecmp(suffix, ".pptx"))
		execlp(DOC_OPEN, DOC_OPEN, argv[1], NULL);
	else if (!strcasecmp(suffix, ".xls"))
		execlp(DOC_OPEN, DOC_OPEN, argv[1], NULL);
	else if (!strcasecmp(suffix, ".xlsx"))
		execlp(DOC_OPEN, DOC_OPEN, argv[1], NULL);
	else if (!strcasecmp(suffix, ".dotx"))
		execlp(DOC_OPEN, DOC_OPEN, argv[1], NULL);

	else
		execlp(DEFAULT, DEFAULT, argv[1], NULL);
}
