/* See LICENSE file for copyright and license details. */

#include <X11/XF86keysym.h>

/* appearance */
/* static const char font[]            		= "-*-terminus-medium-r-*-*-12-*-*-*-*-*-*-*"; */
static const char dmenufont[]           = "-*-ohsnap-medium-r-*-*-12-*-*-*-*-*-*-*";
static const char font[]            		= "-*-ohsnap.icons-medium-r-*-*-12-*-*-*-*-*-*-*";
static const char normbordercolor[] 		= "#191919"; //"#444444";
static const char normbgcolor[]     		= "#191919"; //"#222222";
static const char normfgcolor[]     		= "#c2c2b0"; //"#bbbbbb";
static const char selbordercolor[]  		= "#393939"; //"#005577";
static const char selbgcolor[]      		= "#191919"; //"#005577";
static const char selfgcolor[]      		= "#87877b"; //"#6088bf";
static const char urgbordercolor[]  		= "#bf8860";
static const char topbarbordercolor[] 	= "#292929";
#define NUMCOLORS 8 // need at least 3
static const char* colors[NUMCOLORS][ColLast] = {
   // border   					foreground  	background
   { normbordercolor, 	normfgcolor, 	normbgcolor },  // x01 = normal
   { selbordercolor, 		selfgcolor, 	selbgcolor },  	// x02 = selected
   { urgbordercolor, 		normfgcolor,  normbgcolor },  // x03 = urgent
   { normbordercolor, 	"#ff0000", 		normbgcolor },  // x04 = critical 
   { normbordercolor, 	"#ffbf00", 		normbgcolor },  // x05 = warning 
	 { normbordercolor, 	selfgcolor, 	normbgcolor }, 	// x06 = dimmed
	 { normbordercolor, 	"#ff9200", 		normbgcolor }, 	// x07 = warning < this < critical
	 { normbordercolor,		"#3b4dc4", 		normbgcolor }   // x08 = cold
   // add more here
};
static const unsigned int taglinepx = 1; 				/* height of tag unserline */
static const unsigned int borderpx  = 1;        /* border pixel of windows */
static unsigned int gappx = 6; 									/* gap pixel between windows */
#define MAX_GAPPX 20
static const unsigned int snap      = 32;       /* snap pixel */
static const unsigned int systrayspacing = 2;   /* systray spacing */
static const Bool showsystray       = False;   	/* False means no systray */
static const Bool showbar           = True;     /* False means no bar */
static const Bool topbar            = True;     /* False means bottom bar */
static const Bool viewontag         = True;     /* Switch view on tag switch */
static const Bool triangletag 			= True;     /* Draw triangle instead of square */
static const Bool topbarborder 			= True;     /* Draw border below topbar */

/* False means using the scroll wheel on a window will not change focus */
static const Bool focusonwheelscroll = False;

/* tagging */
#define MAX_TAGLEN 16
static char tags[][MAX_TAGLEN] = { "main", "web", "work", "misc", "media", "chat" };

static const Rule rules[] = {
	/* class      					instance    title       								tags mask     isfloating   monitor */
  /* { NULL,       				NULL,       NULL,       								0,            False,      -1 } */
	{ "Skype",    	 				NULL, 			"martin.ohmann - Skype™", 	1 << 5,       True,       -1 },
	{ "Skype",    	 				NULL, 			NULL, 											1 << 5, 			True,       -1 },
	{ "Pidgin",   	 				NULL,       NULL, 			      					1 << 5,       True,       -1 },
  { "System Monitor", 		NULL, 	 		"SysInfo",  								0,            True,       -1 },
  { "banshee", 						NULL, 	 		"Banshee Media Player", 		1 << 4,   		False,      -1 },
	{ "Thunar",        			NULL,       "File Operation Progress",  0,        		True,    		-1 },
	{ "Thunar",        			NULL,       "Confirm to replace files", 0,        		True,    		-1 },
	{ "Yad",        				NULL,       "System Logout",  					~0,        		True,    		-1 },
	{ "Pavucontrol", 				NULL, 			"Volume Control", 					0, 						True, 			-1 },
	{ "trayer",        			NULL,       NULL,    										~0,        		True,    		-1 },
	{ "stalonetray",    		NULL,       NULL,    										~0,        		True,    		-1 },
	{ "Java", 							NULL, 			"Eclipse", 									0, 						True, 			-1 },
	{ "Java", 							NULL, 			"Eclipse SDK", 							0, 						True, 			-1 }
};

/* layout(s) */
static const float mfact      = 0.50; /* factor of master area size [0.05..0.95] */
static const int nmaster      = 1;    /* number of clients in master area */
static const Bool resizehints = False; /* True means respect size hints in tiled resizals */

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },    /* first entry is default */
	{ "><>",      NULL },    /* no layout function means floating behavior */
	{ "[M]",      monocle },
	{ "[||..]", 	col },
	{ "[|||]", 		tcl },
	{ "[HH]", 		grid },
	{ "[BST]", 		bstack },
	{ "[@]", 			spiral },
	{ "[\\\\]", 	dwindle }
};

/* key definitions */
#define ALTKEY Mod1Mask
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,FKEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} }, \
	{ MODKEY, 											FKEY,     toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static const char scratchpadname[] = "scratchpad";
static const char *scratchpadcmd[] = { "urxvtc", "-name", scratchpadname, "-geometry", "150x40", NULL };
static const char *dmenucmd[] = { "dmenu_run", "-b", "-fn", dmenufont, "-nb", normbgcolor, "-nf", normfgcolor, "-sb", selbgcolor, "-sf", selfgcolor, NULL };
static const char *termcmd[]  = { "urxvtc", NULL };
static const char *thunarcmd[]  = { "thunar", NULL, "Thunar" };
static const char *sysinfocmd[]  = { "toggle-sysinfo", NULL };
static const char *chromiumcmd[]  = { "chromium", NULL, "Chromium" };
static const char *volupcmd[] = { "volup", NULL };
static const char *voldowncmd[] = { "voldown", NULL };
static const char *voltogglecmd[] = { "voltoggle", NULL };
static const char *screensinglecmd[] = { "dwm-scsw", "-s", NULL };
static const char *screenextendcmd[] = { "dwm-scsw", "-s", NULL };
static const char *screenclonecmd[] = { "dwm-scsw", "-s", NULL };
static const char *lockscreencmd[] = { "xscreensaver-command", "-lock", NULL };
static const char *killdwmcmd[] = { "killall", "startdwm", NULL };
static const char *toggletray[] = { "toggletray", NULL };
static const char *logoutdialog[] = { "logout-dialog", NULL, "Logout-Dialog" };
static const char *rangercmd[] = { "urxvtc", "-e", "ranger", NULL };

static Key keys[] = {
	/* modifier                     key        	function        	argument */
	{ MODKEY,                       XK_x,      	spawn,          {.v = dmenucmd } },
	{ MODKEY|ShiftMask,             XK_Return, 	spawn,          {.v = termcmd } },
	{ MODKEY, 	                    XK_w,      	runorraise,     {.v = chromiumcmd } },
	{ MODKEY|ShiftMask, 	          XK_w,      	spawn, 			    {.v = chromiumcmd } },
	{ MODKEY, 	                    XK_e,      	runorraise,     {.v = thunarcmd } },
	{ MODKEY|ShiftMask, 	          XK_e,      	spawn, 			    {.v = thunarcmd } },
	{ MODKEY, 	                    XK_r,      	spawn,          {.v = rangercmd } },
  { 0, 		                        XK_F1,      togglescratch,  {.v = scratchpadcmd } },
	{ MODKEY, 	                    XK_p,      	spawn,          {.v = screenclonecmd } },
	{ MODKEY, 	 	XK_odiaeresis, 								spawn,    	  	{.v = sysinfocmd } },
	{ MODKEY, 	 	XK_adiaeresis, 								spawn,    	  	{.v = toggletray } },
	{ MODKEY, 	 	XK_udiaeresis, 								spawn, 			   	{.v = logoutdialog } },
	{ 0,          XF86XK_AudioMute,             spawn,         	{.v = voltogglecmd } },
	{ 0,          XF86XK_AudioLowerVolume,      spawn,          {.v = voldowncmd } },
	{ 0,          XF86XK_AudioRaiseVolume,      spawn,          {.v = volupcmd } },
	{ 0,         	XF86XK_Launch1,               spawn,          {.v = screensinglecmd } },
	{ 0,          XF86XK_Display,             	spawn,          {.v = screenextendcmd } },
	{ ALTKEY|ControlMask, 					XK_l,      	spawn,          {.v = lockscreencmd } },
	{ MODKEY|ControlMask, 				  XK_q,      	spawn,          {.v = killdwmcmd } },
	{ MODKEY,                       XK_b,      	togglebar,      {0} },
	{ ALTKEY, 	             				XK_Tab,   	focusstack,     {.i = +1 } },
	{ ALTKEY|ShiftMask,   	        XK_Tab,   	focusstack,     {.i = -1 } },
	{ MODKEY,                       XK_j,      	focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_k,      	focusstack,     {.i = -1 } },
	{ MODKEY,                       XK_i,      	incnmaster,     {.i = +1 } },
	{ MODKEY,                       XK_d,      	incnmaster,     {.i = -1 } },
	{ MODKEY,                       XK_h,      	setmfact,       {.f = -0.05} },
	{ MODKEY,                       XK_l,      	setmfact,       {.f = +0.05} },
	{ MODKEY,                       XK_o,      	setgappx,       {.i = +1} },
	{ MODKEY|ShiftMask,             XK_o,      	setgappx,       {.i = -1} },
	{ MODKEY,                       XK_Return, 	zoom,           {0} },
/*{ MODKEY,                       XK_Tab,    	view,           {0} }, */
	{ MODKEY,                       XK_Tab,    	shiftview,      {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_Tab,    	shiftview,      {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_c,      	killclient,     {0} },
	{ ALTKEY,  		           				XK_F4,     	killclient,     {0} },
	{ MODKEY,                       XK_t,      	setlayout,      {.v = &layouts[0]} },
	{ MODKEY,                       XK_f,      	setlayout,      {.v = &layouts[1]} },
	{ MODKEY,                       XK_m,      	setlayout,      {.v = &layouts[2]} },
	{ MODKEY,                       XK_c,      	setlayout,      {.v = &layouts[3]} },
	{ MODKEY,                       XK_v,      	setlayout,      {.v = &layouts[4]} },
	{ MODKEY,                       XK_g,      	setlayout,      {.v = &layouts[5]} },
	{ MODKEY,                       XK_y,      	setlayout,      {.v = &layouts[6]} },
	{ MODKEY,                       XK_s,      	setlayout,      {.v = &layouts[7]} },
	{ MODKEY,                       XK_a,      	setlayout,      {.v = &layouts[8]} },
	{ MODKEY,                       XK_space,  	setlayout,      {0} },
	{ MODKEY|ShiftMask,             XK_space,  	togglefloating, {0} },
	{ MODKEY,                       XK_0,      	view,           {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_0,      	tag,            {.ui = ~0 } },
	{ MODKEY,                       XK_comma,  	focusmon,       {.i = -1 } },
	{ MODKEY,                       XK_period, 	focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_comma,  	tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_period, 	tagmon,         {.i = +1 } },
	{ MODKEY,                       XK_n,      	nametag,        {0} },
	TAGKEYS(                        XK_1, XK_F1,       	        0)
	TAGKEYS(                        XK_2, XK_F2,         	      1)
	TAGKEYS(                        XK_3, XK_F3,           	    2)
	TAGKEYS(                        XK_4, XK_F4,             	  3)
	TAGKEYS(                        XK_5, XK_F5,                4)
	TAGKEYS(                        XK_6, XK_F6,               	5)
	TAGKEYS(                        XK_7, XK_F7,                6)
	TAGKEYS(                        XK_8, XK_F8,                7)
	TAGKEYS(                        XK_9, XK_F9,                8)
	{ MODKEY|ShiftMask,             XK_q,      	quit,           {0} },
};

/* button definitions */
/* click can be ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
	{ ClkWinTitle,        	0,              Button1,        focusstack,     {.i = +1 } },
	{ ClkWinTitle,        	0,              Button3,        zoom,     			{0} },
	{ ClkWinTitle,        	0,         			Button2,        killclient,     {0} },
	{ ClkStatusText,        0,              Button1,        spawn,     			{.v = toggletray } },
	{ ClkStatusText,        0,              Button4,        spawn,     			{.v = volupcmd } },
	{ ClkStatusText,        0,              Button5,        spawn,     			{.v = voldowncmd } },
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

