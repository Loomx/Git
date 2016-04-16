/* config.h for dwm-6.0 */
/* See LICENSE file for copyright and license details. */

/* appearance */
static const char font[]            = "-*-terminus-medium-r-*-*-16-*-*-*-*-*-*-*";
static const char normbordercolor[] = "#000000";
static const char normbgcolor[]     = "#222222";
static const char normfgcolor[]     = "#bbbbbb";
static const char selbordercolor[]  = "#005577";
static const char selbgcolor[]      = "#222222";
static const char selfgcolor[]      = "#22bbee";
static const unsigned int borderpx  = 1;         /* border pixel of windows */
static const unsigned int snap      = 16;        /* snap pixel */
static const Bool showbar           = True;      /* False means no bar */
static const Bool topbar            = True;      /* False means bottom bar */

/* tagging */
static const char *tags[] = { "Here", "There" };

static const Rule rules[] = {
	/* class      instance    title       tags mask     isfloating   monitor */
	{  NULL,      "xv",       NULL,       0,            True,        -1 },
};

/* layout(s) */
static const float mfact      = 0.5; /* factor of master area size [0.05..0.95] */
static const int nmaster      = 1;    /* number of clients in master area */
static const Bool resizehints = False; /* True means respect size hints in tiled resizals */

// #include "bstack.c"
static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[ ]",      monocle },    /* first entry is default */
	{ "[]=",      tile },
//	{ "[-]",      bstack },
};

/* key definitions */
#define MODKEY Mod1Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static const char *dmenucmd[]     = { "dmenu_run", NULL };
static const char *termcmd[]      = { "st", NULL };
static const char *playcmd[]      = { "player", NULL };
static const char *stopcmd[]      = { "player", "stop", NULL };
static const char *prevcmd[]      = { "player", "pt_step", "-1", NULL };
static const char *nextcmd[]      = { "player", "pt_step", "1", NULL };
static const char *mutecmd[]      = { "amixer", "-q", "set", "Master", "toggle", NULL };
//static const char *mutecmd[]      = { "amixer", "-D", "pulse", "-q", "set", "Master", "toggle", NULL };
static const char *ejectcmd[]     = { "eject", NULL };
static const char *sleepcmd[]     = { "sudo", "/usr/sbin/pm-suspend", NULL };

static Key keys[] = {
	/* modifier            key                        function        argument */
	{ 0,                   0xffeb,                    spawn,          {.v = dmenucmd } },
	{ 0,                   0xffec,                    spawn,          {.v = termcmd } },
	{ 0,                   0x1008ff14,                spawn,          {.v = playcmd } },
	{ 0,                   0x1008ff15,                spawn,          {.v = stopcmd } },
	{ 0,                   0x1008ff16,                spawn,          {.v = prevcmd } },
	{ 0,                   0x1008ff17,                spawn,          {.v = nextcmd } },
	{ 0,                   0x1008ff11,                spawn,          SHCMD("amixer set Master 4- unmute | awk -F [][] 'END { print $2 }' >/tmp/volume") },
	{ 0,                   0x1008ff13,                spawn,          SHCMD("amixer set Master 4+ unmute | awk -F [][] 'END { print $2 }' >/tmp/volume") },
//	{ 0,                   0x1008ff11,                spawn,          SHCMD("amixer -D pulse set Master 5%- unmute | awk -F [][] 'END { print $2 }' >/tmp/volume") },
//	{ 0,                   0x1008ff13,                spawn,          SHCMD("amixer -D pulse set Master 5%+ unmute | awk -F [][] 'END { print $2 }' >/tmp/volume") },
	{ 0,                   0x1008ff12,                spawn,          {.v = mutecmd } },
	{ 0,                   0x1008ff2c,                spawn,          {.v = ejectcmd } },
	{ 0,                   0x1008ff2f,                spawn,          {.v = sleepcmd } },
//	{ 0,                   0x1008ff03,                spawn,          SHCMD("Mon=$(cat ~/.mon_brightness); echo $((Mon-440)) >~/.mon_brightness") },
//	{ 0,                   0x1008ff02,                spawn,          SHCMD("Mon=$(cat ~/.mon_brightness); echo $((Mon+440)) >~/.mon_brightness") },
//	{ 0,                   0x1008ff06,                spawn,          SHCMD("Kbd=$(cat ~/.kbd_brightness); echo $((Kbd-2)) >~/.kbd_brightness") },
//	{ 0,                   0x1008ff05,                spawn,          SHCMD("Kbd=$(cat ~/.kbd_brightness); echo $((Kbd+2)) >~/.kbd_brightness") },
	{ MODKEY,              XK_Tab,                    focusstack,     {.i = +1 } },
	{ MODKEY,              XK_space,                  setlayout,      {0} },
	{ MODKEY,              XK_z,                      zoom,           {0} },
	{ MODKEY,              XK_bracketleft,            setmfact,       {.f = -0.05 } },
	{ MODKEY,              XK_bracketright,           setmfact,       {.f = +0.05 } },
	{ MODKEY,              XK_q,                      killclient,     {0} },
	TAGKEYS(               XK_comma,                                  0)
	TAGKEYS(               XK_period,                                 1)
};

/* button definitions */
/* click can be ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkWinTitle,          0,              Button1,        spawn,          {.v = dmenucmd } },
	{ ClkWinTitle,          0,              Button3,        spawn,          {.v = termcmd } },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            ControlMask,    Button1,        toggleview,     {0} },
	{ ClkTagBar,            0,              Button3,        tag,            {0} },
	{ ClkTagBar,            ControlMask,    Button3,        toggletag,      {0} },
};

