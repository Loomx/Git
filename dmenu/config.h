/* See LICENSE file for copyright and license details. */
/* vim: expandtab
 */
/* Default settings; can be overriden by command line. */

#define CACHE  ".cache/dmenu_run"

static int topbar = 1;
static const char *font = "-*-terminus-medium-r-*-*-16-*-*-*-*-*-*-*";
static const char *prompt = NULL;
static const char *normbgcolor = "#222222";
static const char *normfgcolor = "#bbbbbb";
static const char *selbgcolor  = "#222222";
static const char *selfgcolor  = "#2288ee";
static const char *outbgcolor  = "#00ffff";
static const char *outfgcolor  = "#000000";
/* -l option; if nonzero, dmenu uses vertical list with given number of lines */
static unsigned int lines = 0;
static int centered = 0;         /* -c option; centers dmenu on screen */
static int min_width = 400;      /*    minimum width when centered     */
