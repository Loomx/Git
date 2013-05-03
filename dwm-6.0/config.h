/* Jonny's config.h for dwm-6.0 */
/* See LICENSE file for copyright and license details. */

/* appearance */
static const char font[]            = "-*-terminus-medium-r-*-*-16-*-*-*-*-*-*-*";
static const char normbordercolor[] = "#444444";    /* #000000 */
static const char normbgcolor[]     = "#222222";    /* #cccccc */
static const char normfgcolor[]     = "#bbbbbb";    /* #000000 */
static const char selbordercolor[]  = "#005577";    /* #cccccc */
static const char selbgcolor[]      = "#005577";    /* #0066ff */
static const char selfgcolor[]      = "#eeeeee";    /* #ffffff */
static const unsigned int borderpx  = 1;         /* border pixel of windows */
static const unsigned int snap      = 32;        /* snap pixel */
static const Bool showbar           = True;      /* False means no bar */
static const Bool topbar            = True;      /* False means bottom bar */

/* tagging */
static const char *tags[] = { "Here", "There" };

static const Rule rules[] = {
    /* class      instance    title       tags mask     isfloating   monitor */
    { "Gimp",     NULL,       NULL,       0,            True,        -1 },
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
//  { "[-]",      bstack },
};

/* key definitions */
#define MODKEY Mod1Mask
#define TAGKEYS(KEY,TAG) \
    { MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
    { MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },
#include <X11/XF86keysym.h>

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static const char *dmenucmd[]     = { "dmenu_run", NULL };
static const char *termcmd[]      = { "xterm", NULL };
static const char *mutecmd[]      = { "amixer", "-q", "set", "Master", "toggle", NULL };
static const char *ejectcmd[]     = { "eject", NULL };

static Key keys[] = {
    /* modifier            key                        function        argument */
    { 0,                   XF86XK_Launch1,            spawn,          {.v = dmenucmd } },
    { 0,                   XF86XK_Launch2,            spawn,          {.v = termcmd } },
    { 0,                   XF86XK_AudioPlay,          spawn,          SHCMD("pgrep mplayer >/dev/null && echo pause >~/.mplayer/mp_pipe || player") },
    { 0,                   XF86XK_AudioStop,          spawn,          SHCMD("pgrep mplayer >/dev/null && echo stop >~/.mplayer/mp_pipe") },
    { 0,                   XF86XK_AudioPrev,          spawn,          SHCMD("pgrep mplayer >/dev/null && echo pt_step -1 >~/.mplayer/mp_pipe") },
    { 0,                   XF86XK_AudioNext,          spawn,          SHCMD("pgrep mplayer >/dev/null && echo pt_step 1 >~/.mplayer/mp_pipe") },
    { 0,                   XF86XK_AudioLowerVolume,   spawn,          SHCMD("amixer set Master 4- unmute | awk -F [][] 'END { print $2 }' >/tmp/alsa_volume") },
    { 0,                   XF86XK_AudioRaiseVolume,   spawn,          SHCMD("amixer set Master 4+ unmute | awk -F [][] 'END { print $2 }' >/tmp/alsa_volume") },
    { 0,                   XF86XK_AudioMute,          spawn,          {.v = mutecmd } },
    { 0,                   XF86XK_Eject,              spawn,          {.v = ejectcmd } },
    { 0,                   XF86XK_MonBrightnessDown,  spawn,          SHCMD("echo 8 >~/.mon_brightness") },
    { 0,                   XF86XK_MonBrightnessUp,    spawn,          SHCMD("echo 15 >~/.mon_brightness") },
    { 0,                   XF86XK_KbdBrightnessDown,  spawn,          SHCMD("echo 0 >~/.kbd_brightness") },
    { 0,                   XF86XK_KbdBrightnessUp,    spawn,          SHCMD("echo 2 >~/.kbd_brightness") },
    { MODKEY,              XK_Tab,                    focusstack,     {.i = +1 } },
    { MODKEY,              XK_space,                  setlayout,      {0} },
    { MODKEY,              XK_z,                      zoom,           {0} },
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

