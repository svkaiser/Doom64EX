// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2007-2012 Samuel Villarreal
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//      DOOM selection menu, options, episode etc.
//      Sliders and icons. Kinda widget stuff.
//
//-----------------------------------------------------------------------------

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifdef _MSC_VER
#include "i_opndir.h"
#else
#include <dirent.h>
#endif

#include <fcntl.h>
#include "doomdef.h"
#include "i_video.h"
#include "d_englsh.h"
#include "m_cheat.h"
#include "m_misc.h"
#include "d_main.h"
#include "i_system.h"
#include "z_zone.h"
#include "st_stuff.h"
#include "g_actions.h"
#include "g_game.h"
#include "s_sound.h"
#include "doomstat.h"
#include "sounds.h"
#include "m_menu.h"
#include "m_fixed.h"
#include "d_devstat.h"
#include "r_local.h"
#include "m_shift.h"
#include "m_password.h"
#include "r_wipe.h"
#include "st_stuff.h"
#include "p_saveg.h"
#include "p_setup.h"
#include "gl_texture.h"
#include "gl_draw.h"
#include <imp/Wad>
#include <imp/Video>
//
// definitions
//

#define MENUSTRINGSIZE      32
#define SKULLXOFF           -40    //villsa: changed from -32 to -40
#define SKULLXTEXTOFF       -24
#define LINEHEIGHT          18
#define TEXTLINEHEIGHT      18
#define MENUCOLORRED        D_RGBA(255, 0, 0, menualphacolor)
#define MENUCOLORWHITE        D_RGBA(255, 255, 255, menualphacolor)
#define MAXBRIGHTNESS        100

//
// defaulted values
//

dboolean            allowmenu = true;                   // can menu be accessed?
dboolean            menuactive = false;
dboolean            mainmenuactive = false;
dboolean            allowclearmenu = true;              // can user hit escape to clear menu?

static dboolean     newmenu = false;    // 20120323 villsa
static char         *messageBindCommand;
static int          quickSaveSlot;                      // -1 = no quicksave slot picked!
static int          saveSlot;                           // which slot to save in
static char         savegamestrings[10][MENUSTRINGSIZE];
static dboolean     alphaprevmenu = false;
static int          menualphacolor = 0xff;

static char         inputString[MENUSTRINGSIZE];
static char         oldInputString[MENUSTRINGSIZE];
static dboolean     inputEnter = false;
static int          inputCharIndex;
static int          inputMax = 0;

//
// fade-in/out stuff
//

void M_MenuFadeIn(void);
void M_MenuFadeOut(void);
void(*menufadefunc)(void) = NULL;

//
// static variables
//

static char     MenuBindBuff[256];
static char     MenuBindMessage[256];
static dboolean MenuBindActive = false;
static dboolean showfullitemvalue[3] = { false, false, false};
static int      levelwarp = 0;
static dboolean wireframeon = false;
static int      thermowait = 0;
static int      m_ScreenSize = 1;

//------------------------------------------------------------------------
//
// MENU TYPEDEFS
//
//------------------------------------------------------------------------

typedef struct {
    // -3 = disabled/hidden item, -2 = enter key ok, -1 = disabled, 0 = no cursor here,
    // 1 = ok, 2 = arrows ok, 3 = for sliders
    short status;

    char name[64];

    // choice = menu item #
    void (*routine)(int choice);

    // hotkey in menu
    char alphaKey;
} menuitem_t;

typedef struct {
    Property *mitem;
    float    mdefault;
} menudefault_t;

typedef struct {
    int item;
    float width;
    Property *mitem;
} menuthermobar_t;

typedef struct menu_s {
    short               numitems;           // # of menu items
    dboolean            textonly;
    struct menu_s       *prevMenu;          // previous menu
    menuitem_t          *menuitems;         // menu items
    void (*routine)(void);                  // draw routine
    char                title[64];
    short               x;
    short               y;                  // x,y of menu
    short               lastOn;             // last item user was on in menu
    dboolean            smallfont;          // draw text using small fonts
    menudefault_t       *defaultitems;      // pointer to default values for cvars
    short               numpageitems;       // number of items to display per page
    short               menupageoffset;
    float               scale;
    const char          **hints;
    menuthermobar_t     *thermobars;
} menu_t;

typedef struct {
    const char    *name;
    const char    *action;
} menuaction_t;

short           itemOn;                 // menu item skull is on
short           itemSelected;
short           skullAnimCounter;       // skull animation counter
short           whichSkull;             // which skull to draw

char    msgNames[2][4]          = {"Off","On"};

// current menudef
static menu_t* currentMenu;
static menu_t* nextmenu;

extern menu_t ControlMenuDef;

//------------------------------------------------------------------------
//
// PROTOTYPES
//
//------------------------------------------------------------------------

void M_SetupNextMenu(menu_t *menudef);
void M_ClearMenus(void);

void M_QuickSave(void);
void M_QuickLoad(void);

static int M_StringWidth(const char *string);
static int M_BigStringWidth(const char *string);

static void M_DrawThermo(int x, int y, int thermWidth, float thermDot);
static void M_DoDefaults(int choice);
static void M_Return(int choice);
static void M_ReturnToOptions(int choice);
template <class T, class U>
static void M_SetCvar(TypedProperty<T> &property, const U &value);
template <class T, class U>
static void M_SetOptionValue(int choice, U min, U max, U inc, TypedProperty<T> &cvar);
static void M_SetOptionValue(int choice, BoolProperty &cvar);
static void M_DrawSmbString(const char* text, menu_t* menu, int item);
static void M_DrawSaveGameFrontend(menu_t* def);
static void M_SetInputString(char* string, int len);
static void M_Scroll(menu_t* menu, dboolean up);

static dboolean M_SetThumbnail(int which);

IntProperty m_regionblood("m_regionblood", "");

FloatProperty m_menufadetime("m_menufadetime", "", 0.0f, 0,
                             [](const FloatProperty &p, float, float& x)
                             {
                                 x = clamp(*p, 0.0f, 80.0f);
                             });

BoolProperty m_menumouse("m_menumouse", "", true, 0,
                         [](const BoolProperty &p, bool, bool&) {
                             SDL_ShowCursor(*p ? SDL_FALSE : SDL_TRUE);
                             if(!*p) {
                                 itemSelected = -1;
                             }
                         });

FloatProperty m_cursorscale("m_cursorscale", "", 8.0f, 0,
                            [](const FloatProperty &p, float, float &x)
                            {
                                x = clamp(*p, 0.0f, 50.0f);
                            });

//------------------------------------------------------------------------
//
// MAIN MENU
//
//------------------------------------------------------------------------

void M_NewGame(int choice);
void M_Options(int choice);
void M_LoadGame(int choice);
void M_QuitDOOM(int choice);
void M_QuitDOOM2(int choice);

enum {
    newgame = 0,
    options,
    loadgame,
    quitdoom,
    main_end
};

menuitem_t MainMenu[]= {
    {1,"New Game",M_NewGame,'n'},
    {1,"Options",M_Options,'o'},
    {1,"Load Game",M_LoadGame,'l'},
    {1,"Quit Game",M_QuitDOOM2,'q'},
};

menu_t MainDef = {
    main_end,
    false,
    NULL,
    MainMenu,
    NULL,
    "",
    112,150,
    0,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

//------------------------------------------------------------------------
//
// IN GAME MENU
//
//------------------------------------------------------------------------

void M_SaveGame(int choice);
void M_Features(int choice);
void M_QuitMainMenu(int choice);
void M_ConfirmRestart(int choice);

enum {
    pause_options = 0,
    pause_mainmenu,
    pause_restartlevel,
    pause_features,
    pause_loadgame,
    pause_savegame,
    pause_quitdoom,
    pause_end
};

menuitem_t PauseMenu[]= {
    {1,"Options",M_Options,'o'},
    {1,"Main Menu",M_QuitMainMenu,'m'},
    {1,"Restart Level",M_ConfirmRestart,'r'},
    {-3,"Features",M_Features,'f'},
    {1,"Load Game",M_LoadGame,'l'},
    {1,"Save Game",M_SaveGame,'s'},
    {1,"Quit Game",M_QuitDOOM,'q'},
};

menu_t PauseDef = {
    pause_end,
    false,
    NULL,
    PauseMenu,
    NULL,
    "Pause",
    112,80,
    0,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

//------------------------------------------------------------------------
//
// QUIT GAME PROMPT
//
//------------------------------------------------------------------------

void M_QuitGame(int choice);
void M_QuitGameBack(int choice);

enum {
    quityes = 0,
    quitno,
    quitend
};

menuitem_t QuitGameMenu[]= {
    {1,"Yes",M_QuitGame,'y'},
    {1,"No",M_QuitGameBack,'n'},
};

menu_t QuitDef = {
    quitend,
    false,
    &PauseDef,
    QuitGameMenu,
    NULL,
    "Quit DOOM?",
    144,112,
    quitno,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

void M_QuitDOOM(int choice) {
    M_SetupNextMenu(&QuitDef);
}

void M_QuitGame(int choice) {
    I_Quit();
}

void M_QuitGameBack(int choice) {
    M_SetupNextMenu(currentMenu->prevMenu);
}

//------------------------------------------------------------------------
//
// QUIT GAME PROMPT (From Main Menu)
//
//------------------------------------------------------------------------

void M_QuitGame2(int choice);
void M_QuitGameBack2(int choice);

enum {
    quit2yes = 0,
    quit2no,
    quit2end
};

menuitem_t QuitGameMenu2[]= {
    {1,"Yes",M_QuitGame2,'y'},
    {1,"No",M_QuitGameBack2,'n'},
};

menu_t QuitDef2 = {
    quit2end,
    false,
    &MainDef,
    QuitGameMenu2,
    NULL,
    "Quit DOOM?",
    144,112,
    quit2no,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

void M_QuitDOOM2(int choice) {
    M_SetupNextMenu(&QuitDef2);
}

void M_QuitGame2(int choice) {
    I_Quit();
}

void M_QuitGameBack2(int choice) {
    M_SetupNextMenu(currentMenu->prevMenu);
}

//------------------------------------------------------------------------
//
// EXIT TO MAIN MENU PROMPT
//
//------------------------------------------------------------------------

void M_EndGame(int choice);

enum {
    PMainYes = 0,
    PMainNo,
    PMain_end
};

menuitem_t PromptMain[]= {
    {1,"Yes",M_EndGame,'y'},
    {1,"No",M_ReturnToOptions,'n'},
};

menu_t PromptMainDef = {
    PMain_end,
    false,
    &PauseDef,
    PromptMain,
    NULL,
    "Quit To Main Menu?",
    144,112,
    PMainNo,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

void M_QuitMainMenu(int choice) {
    M_SetupNextMenu(&PromptMainDef);
}

void M_EndGame(int choice) {
    if(choice) {
        currentMenu->lastOn = itemOn;
        if(currentMenu->prevMenu) {
            menufadefunc = M_MenuFadeOut;
            alphaprevmenu = true;
        }
    }
    else {
        currentMenu->lastOn = itemOn;
        M_ClearMenus();
        gameaction = ga_title;
        currentMenu = &MainDef;
    }
}

//------------------------------------------------------------------------
//
// RESTART LEVEL PROMPT
//
//------------------------------------------------------------------------

void M_RestartLevel(int choice);

enum {
    RMainYes = 0,
    RMainNo,
    RMain_end
};

menuitem_t RestartConfirmMain[]= {
    {1,"Yes",M_RestartLevel,'y'},
    {1,"No",M_ReturnToOptions,'n'},
};

menu_t RestartDef = {
    RMain_end,
    false,
    &PauseDef,
    RestartConfirmMain,
    NULL,
    "Quit Current Game?",
    144,112,
    RMainNo,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

void M_ConfirmRestart(int choice) {
    M_SetupNextMenu(&RestartDef);
}

void M_RestartLevel(int choice) {
    if(!netgame) {
        gameaction = ga_loadlevel;
        nextmap = gamemap;
        players[consoleplayer].playerstate = PST_REBORN;
    }

    currentMenu->lastOn = itemOn;
    M_ClearMenus();
}

//------------------------------------------------------------------------
//
// START NEW IN NETGAME NOTIFY
//
//------------------------------------------------------------------------

void M_DrawStartNewNotify(void);
void M_NewGameNotifyResponse(int choice);

enum {
    SNN_Ok = 0,
    SNN_End
};

menuitem_t StartNewNotify[]= {
    {1,"Ok",M_NewGameNotifyResponse,'o'}
};

menu_t StartNewNotifyDef = {
    SNN_End,
    false,
    &PauseDef,
    StartNewNotify,
    M_DrawStartNewNotify,
    " ",
    144,112,
    SNN_Ok,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

void M_NewGameNotifyResponse(int choice) {
    M_SetupNextMenu(&MainDef);
}

void M_DrawStartNewNotify(void) {
    Draw_BigText(-1, 16, MENUCOLORRED , "You Cannot Start");
    Draw_BigText(-1, 32, MENUCOLORRED , "A New Game On A Network");
}

//------------------------------------------------------------------------
//
// NEW GAME MENU
//
//------------------------------------------------------------------------

void M_ChooseSkill(int choice);

enum {
    killthings,
    toorough,
    hurtme,
    violence,
    nightmare,
    newg_end
};

menuitem_t NewGameMenu[]= {
    {1,"Be Gentle!",M_ChooseSkill, 'b'},
    {1,"Bring It On!",M_ChooseSkill, 'r'},
    {1,"I Own Doom!",M_ChooseSkill, 'i'},
    {1,"Watch Me Die!",M_ChooseSkill, 'w'},
    {-3,"Hardcore!",M_ChooseSkill, 'h'},
};

menu_t NewDef = {
    newg_end,
    false,
    &MainDef,
    NewGameMenu,
    NULL,
    "Choose Your Skill...",
    112,80,
    toorough,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

void M_NewGame(int choice) {
    if(netgame && !demoplayback) {
        M_StartControlPanel(true);
        M_SetupNextMenu(&StartNewNotifyDef);
        return;
    }

    M_SetupNextMenu(&NewDef);
}

void M_ChooseSkill(int choice) {
    G_DeferedInitNew(choice,1);
    M_ClearMenus();
    dmemset(passwordData, 0xff, 16);
    allowmenu = false;
}


//------------------------------------------------------------------------
//
// OPTIONS MENU
//
//------------------------------------------------------------------------

void M_DrawOptions(void);
void M_Controls(int choice);
void M_Sound(int choice);
void M_Display(int choice);
void M_Video(int choice);
void M_Misc(int choice);
void M_Password(int choice);
void M_Network(int choice);
void M_Region(int choice);

enum {
    options_controls,
    options_misc,
    options_soundvol,
    options_display,
    options_video,
    options_password,
    options_network,
    options_region,
    options_return,
    opt_end
};

menuitem_t OptionsMenu[]= {
    {1,"Controls",M_Controls, 'c'},
    {1,"Setup",M_Misc, 'e'},
    {1,"Sound",M_Sound,'s'},
    {1,"Display",M_Display, 'd'},
    {1,"Video",M_Video, 'v'},
    {1,"Password",M_Password, 'p'},
    {1,"Network",M_Network, 'n'},
    {1,"Region",M_Region, 'f'},
    {1,"/r Return",M_Return, 0x20}
};

const char* OptionHints[opt_end]= {
    "control configuration",
    "miscellaneous options for gameplay and other features",
    "adjust sound volume",
    "settings for heads up display",
    "configure video-specific options",
    "enter a password to access a level",
    "setup options for a hosted session",
    "configure localization settings",
    NULL
};

menu_t OptionsDef = {
    opt_end,
    false,
    &PauseDef,
    OptionsMenu,
    M_DrawOptions,
    "Options",
    170,80,
    0,
    false,
    NULL,
    -1,
    0,
    0.75f,
    OptionHints,
    NULL
};

void M_Options(int choice) {
    M_SetupNextMenu(&OptionsDef);
}

void M_DrawOptions(void) {
    if(OptionsDef.hints[itemOn] != NULL) {
        GL_SetOrthoScale(0.5f);
        Draw_BigText(-1, 410, MENUCOLORWHITE, OptionsDef.hints[itemOn]);
        GL_SetOrthoScale(OptionsDef.scale);
    }
}


//------------------------------------------------------------------------
//
// REGIONAL MENU
//
//------------------------------------------------------------------------

void M_RegionChoice(int choice);
void M_DrawRegion(void);

extern BoolProperty st_regionmsg;
extern IntProperty m_regionblood;
extern IntProperty p_regionmode;

enum {
    region_mode,
    region_lang,
    region_blood,
    region_default,
    region_return,
    region_end
};

menuitem_t RegionMenu[]= {
    {2,"Region Mode:", M_RegionChoice, 'r'},
    {2,"Language:",M_RegionChoice, 'l'},
    {2,"Blood Color:",M_RegionChoice, 'b'},
    {-2,"Default",M_DoDefaults,'d'},
    {1,"/r Return",M_Return, 0x20}
};

menudefault_t RegionDefault[] = {
    { &p_regionmode, 0 },
    { &st_regionmsg, 0 },
    { &m_regionblood, 0 },
    { NULL, -1 }
};

const char* RegionHints[region_end]= {
    "affects the legal screen",
    "change language for hud messages",
    "changes color for blood",
    NULL,
    NULL
};

menu_t RegionDef = {
    region_end,
    false,
    &OptionsDef,
    RegionMenu,
    M_DrawRegion,
    "Region",
    130,80,
    0,
    false,
    RegionDefault,
    -1,
    0,
    0.8f,
    RegionHints,
    NULL
};

void M_Region(int choice) {
    M_SetupNextMenu(&RegionDef);
}

void M_RegionChoice(int choice) {
    int lump;

    switch(itemOn) {
    case region_mode:
        //
        // 20120304 villsa
        //
        if(RegionMenu[region_mode].status == 1) {
            return;
        }

        M_SetOptionValue(choice, 0, 2, 1, p_regionmode);
        break;

    case region_lang:
        //
        // 20120304 villsa
        //
        if(RegionMenu[region_lang].status == 1) {
            return;
        }

        M_SetOptionValue(choice, 0, 1, 1, st_regionmsg);
        break;

    case region_blood:
        //
        // 20120304 villsa
        //
        if(RegionMenu[region_blood].status == 1) {
            return;
        }

        M_SetOptionValue(choice, 0, 1, 1, m_regionblood);

        lump = *m_regionblood;

        if(lump > 1) {
            lump = 1;
        }
        if(lump < 0) {
            lump = 0;
        }

        states[S_494].sprite = SPR_RBLD + lump;
        states[S_495].sprite = SPR_RBLD + lump;
        states[S_496].sprite = SPR_RBLD + lump;
        states[S_497].sprite = SPR_RBLD + lump;

        break;
    }
}

void M_DrawRegion(void) {
    static const char* bloodcolor[2]    = { "Red", "Green" };
    static const char* language[2]      = { "English", "Japanese" };
    static const char* regionmode[3]    = { "NTSC", "PAL", "NTSC-Japan" };
    const char* string;
    rcolor color;

    if(RegionMenu[region_mode].status == 1) {
        color = MENUCOLORWHITE;
        string = "Unavailable";
    }
    else {
        color = MENUCOLORRED;
        string = (char*)regionmode[*p_regionmode];
    }

    Draw_BigText(RegionDef.x + 136, RegionDef.y + LINEHEIGHT * region_mode, color, string);

    if(RegionMenu[region_lang].status == 1) {
        color = MENUCOLORWHITE;
        string = "Unavailable";
    }
    else {
        color = MENUCOLORRED;
        string = language[*st_regionmsg];
    }

    Draw_BigText(RegionDef.x + 136, RegionDef.y + LINEHEIGHT * region_lang, color, string);

    if(RegionMenu[region_blood].status == 1) {
        color = MENUCOLORWHITE;
        string = "Unavailable";
    }
    else {
        color = MENUCOLORRED;
        string = bloodcolor[*m_regionblood];
    }

    Draw_BigText(RegionDef.x + 136, RegionDef.y + LINEHEIGHT * region_blood, color, string);

    if(RegionDef.hints[itemOn] != NULL) {
        GL_SetOrthoScale(0.5f);
        Draw_BigText(-1, 410, MENUCOLORWHITE, RegionDef.hints[itemOn]);
        GL_SetOrthoScale(RegionDef.scale);
    }
}

//------------------------------------------------------------------------
//
// NETWORK MENU
//
//------------------------------------------------------------------------

void M_NetworkChoice(int choice);
void M_PlayerSetName(int choice);
void M_DrawNetwork(void);

extern StringProperty m_playername;
extern BoolProperty p_allowjump;
extern BoolProperty p_autoaim;
extern BoolProperty sv_nomonsters;
extern BoolProperty sv_fastmonsters;
extern BoolProperty sv_respawnitems;
extern BoolProperty sv_respawn;
extern BoolProperty sv_allowcheats;
extern BoolProperty sv_friendlyfire;
extern BoolProperty sv_keepitems;

enum {
    network_header1,
    network_playername,
    network_header2,
    network_allowcheats,
    network_friendlyfire,
    network_keepitems,
    network_allowjump,
    network_allowautoaim,
    network_header3,
    network_nomonsters,
    network_fastmonsters,
    network_respawnmonsters,
    network_respawnitems,
    network_default,
    network_return,
    network_end
};

menuitem_t NetworkMenu[]= {
    {-1,"Player Setup",0 },
    {1,"Player Name:", M_PlayerSetName, 'p'},
    {-1,"Player Rules",0 },
    {2,"Allow Cheats:", M_NetworkChoice, 'c'},
    {2,"Friendly Fire:", M_NetworkChoice, 'f'},
    {2,"Keep Items:", M_NetworkChoice, 'k'},
    {2,"Allow Jumping:", M_NetworkChoice, 'j'},
    {2,"Allow Auto Aiming:", M_NetworkChoice, 'a'},
    {-1,"Gameplay Rules",0 },
    {2,"No Monsters:", M_NetworkChoice, 'n'},
    {2,"Fast Monsters:", M_NetworkChoice, 'f'},
    {2,"Respawn Monsters:", M_NetworkChoice, 'r'},
    {2,"Respawn Items:", M_NetworkChoice, 'i'},
    {-2,"Default",M_DoDefaults,'d'},
    {1,"/r Return",M_Return, 0x20}
};

menudefault_t NetworkDefault[] = {
    { &sv_allowcheats, 0 },
    { &sv_friendlyfire, 0 },
    { &sv_keepitems, 0 },
    { &p_allowjump, 0 },
    { &p_autoaim, 1 },
    { &sv_nomonsters, 0 },
    { &sv_fastmonsters, 0 },
    { &sv_respawn, 0 },
    { &sv_respawnitems, 0 },
    { NULL, -1 }
};

const char* NetworkHints[network_end]= {
    NULL,
    "set a name for yourself",
    NULL,
    "allow clients and host to use cheats",
    "allow players to damage other players",
    "players keep items when respawned from death",
    "allow players to jump",
    "enable or disable auto aiming for all players",
    NULL,
    "no monsters will appear",
    "increased speed for monsters and projectiles",
    "monsters will respawn after death",
    "items will respawn after pickup",
    NULL,
    NULL
};

menu_t NetworkDef = {
    network_end,
    false,
    &OptionsDef,
    NetworkMenu,
    M_DrawNetwork,
    "Network",
    208,108,
    0,
    false,
    NetworkDefault,
    14,
    0,
    0.5f,
    NetworkHints,
    NULL
};

void M_Network(int choice) {
    M_SetupNextMenu(&NetworkDef);
    dstrcpy(inputString, static_cast<String>(m_playername).c_str());
}

void M_PlayerSetName(int choice) {
    // FIXME: Allow inputting player name
    char string[16] = "DUMMY TEXT";
    M_SetInputString(string, 16);
}

void M_NetworkChoice(int choice) {
    switch(itemOn) {
    case network_allowcheats:
        M_SetOptionValue(choice, sv_allowcheats);
        break;
    case network_friendlyfire:
        M_SetOptionValue(choice, sv_friendlyfire);
        break;
    case network_keepitems:
        M_SetOptionValue(choice, sv_keepitems);
        break;
    case network_allowjump:
        M_SetOptionValue(choice, p_allowjump);
        break;
    case network_allowautoaim:
        M_SetOptionValue(choice, p_autoaim);
        break;
    case network_nomonsters:
        M_SetOptionValue(choice, sv_nomonsters);
        break;
    case network_fastmonsters:
        M_SetOptionValue(choice, sv_fastmonsters);
        break;
    case network_respawnmonsters:
        M_SetOptionValue(choice, sv_respawn);
        break;
    case network_respawnitems:
        M_SetOptionValue(choice, 0, 10, 1, sv_respawnitems);
        break;
    }
}

void M_DrawNetwork(void) {
    int y;
    static const char* respawnitemstrings[11] = {
        "Off",
        "1 Minute",
        "2 Minutes",
        "3 Minutes",
        "4 Minutes",
        "5 Minutes",
        "6 Minutes",
        "7 Minutes",
        "8 Minutes",
        "9 Minutes",
        "10 Minutes"
    };

#define DRAWNETWORKITEM(a, b, c) \
    if(currentMenu->menupageoffset <= a && \
        a - currentMenu->menupageoffset < currentMenu->numpageitems) \
    { \
        y = a - currentMenu->menupageoffset; \
        Draw_BigText(NetworkDef.x + 194, NetworkDef.y+LINEHEIGHT*y, MENUCOLORRED, \
            c[(int)b]); \
    }

    DRAWNETWORKITEM(network_playername, 0, &inputString);
    DRAWNETWORKITEM(network_allowcheats, *sv_allowcheats, msgNames);
    DRAWNETWORKITEM(network_friendlyfire, *sv_friendlyfire, msgNames);
    DRAWNETWORKITEM(network_keepitems, *sv_keepitems, msgNames);
    DRAWNETWORKITEM(network_allowjump, *p_allowjump, msgNames);
    DRAWNETWORKITEM(network_allowautoaim, *p_autoaim, msgNames);
    DRAWNETWORKITEM(network_nomonsters, *sv_nomonsters, msgNames);
    DRAWNETWORKITEM(network_fastmonsters, *sv_fastmonsters, msgNames);
    DRAWNETWORKITEM(network_respawnmonsters, *sv_respawn, msgNames);
    DRAWNETWORKITEM(network_respawnitems, *sv_respawnitems, respawnitemstrings);

#undef DRAWNETWORKITEM

    if(inputEnter) {
        int i;

        y = network_playername - currentMenu->menupageoffset;
        i = ((int)(160.0f / NetworkDef.scale) - Center_Text(inputString)) * 2;
        Draw_BigText(NetworkDef.x + i + 198, (NetworkDef.y + LINEHEIGHT * y), MENUCOLORWHITE, "/r");
    }

    if(NetworkDef.hints[itemOn] != NULL) {
        GL_SetOrthoScale(0.5f);
        Draw_BigText(-1, 410, MENUCOLORWHITE, NetworkDef.hints[itemOn]);
        GL_SetOrthoScale(NetworkDef.scale);
    }
}

//------------------------------------------------------------------------
//
// MISC MENU
//
//------------------------------------------------------------------------

void M_MiscChoice(int choice);
void M_DrawMisc(void);

extern BoolProperty am_showkeymarkers;
extern BoolProperty am_showkeycolors;
extern BoolProperty am_drawobjects;
extern BoolProperty am_overlay;
extern BoolProperty r_skybox;
extern BoolProperty r_texnonpowresize;
extern BoolProperty i_interpolateframes;
extern BoolProperty p_usecontext;
extern BoolProperty compat_collision;
extern BoolProperty compat_limitpain;
extern BoolProperty compat_mobjpass;
extern BoolProperty compat_grabitems;
extern BoolProperty r_wipe;
extern BoolProperty r_rendersprites;
extern BoolProperty r_texturecombiner;
extern IntProperty r_colorscale;

enum {
    misc_header1,
    misc_menufade,
    misc_empty1,
    misc_menumouse,
    misc_cursorsize,
    misc_empty2,
    misc_header2,
    misc_aim,
    misc_jump,
    misc_context,
    misc_header3,
    misc_wipe,
    misc_texresize,
    misc_frame,
    misc_combine,
    misc_sprites,
    misc_skybox,
    misc_rgbscale,
    misc_header4,
    misc_showkey,
    misc_showlocks,
    misc_amobjects,
    misc_amoverlay,
    misc_header5,
    misc_comp_collision,
    misc_comp_pain,
    misc_comp_pass,
    misc_comp_grab,
    misc_default,
    misc_return,
    misc_end
};

menuitem_t MiscMenu[]= {
    {-1,"Menu Options",0 },
    {3,"Menu Fade Speed",M_MiscChoice, 'm' },
    {-1,"",0 },
    {2,"Show Cursor:",M_MiscChoice, 'h'},
    {3,"Cursor Scale:",M_MiscChoice,'u'},
    {-1,"",0 },
    {-1,"Gameplay",0 },
    {2,"Auto Aim:",M_MiscChoice, 'a'},
    {2,"Jumping:",M_MiscChoice, 'j'},
    {2,"Use Context:",M_MiscChoice, 'u'},
    {-1,"Rendering",0 },
    {2,"Screen Melt:",M_MiscChoice, 's' },
    {2,"Texture Fit:",M_MiscChoice,'t' },
    {2,"Framerate:",M_MiscChoice, 'f' },
    {2,"Use Combiners:",M_MiscChoice, 'c' },
    {2,"Sprite Pitch:",M_MiscChoice,'p'},
    {2,"Skybox:",M_MiscChoice,'k'},
    {2,"Color Scale:",M_MiscChoice,'o'},
    {-1,"Automap",0 },
    {2,"Key Pickups:",M_MiscChoice },
    {2,"Locked Doors:",M_MiscChoice },
    {2,"Draw Objects:",M_MiscChoice },
    {2,"Overlay:",M_MiscChoice },
    {-1,"N64 Compatibility",0 },
    {2,"Collision:",M_MiscChoice,'c' },
    {2,"Limit Lost Souls:",M_MiscChoice,'l'},
    {2,"Tall Actors:",M_MiscChoice,'i'},
    {2,"Grab High Items:",M_MiscChoice,'g'},
    {-2,"Default",M_DoDefaults,'d'},
    {1,"/r Return",M_Return, 0x20}
};

const char* MiscHints[misc_end]= {
    NULL,
    "change transition speeds between switching menus",
    NULL,
    "enable menu cursor",
    "set the size of the mouse cursor",
    NULL,
    NULL,
    "toggle classic style auto-aiming",
    "toggle the ability to jump",
    "if enabled interactive objects will highlight when near",
    NULL,
    "enable the melt effect when completing a level",
    "set how texture dimentions are stretched",
    "interpolate between frames to achieve smooth framerate",
    "use texture combining - not supported by low-end cards",
    "toggles billboard sprite rendering",
    "toggle skies to render either normally or as skyboxes",
    "scales the overall color RGB",
    NULL,
    "display key pickups in automap",
    "colorize locked doors accordingly to the key in automap",
    "set how objects are rendered in automap",
    "render the automap into the player hud",
    NULL,
    "surrounding blockmaps are not checked for an object",
    "limit max amount of lost souls spawned by pain elemental to 17",
    "emulate infinite height bug for all solid actors",
    "be able to grab high items by bumping into the sector it sits on",
    NULL,
    NULL
};

menudefault_t MiscDefault[] = {
    { &m_menufadetime, 0 },
    { &m_menumouse, 1 },
    { &m_cursorscale, 8 },
    { &p_autoaim, 1 },
    { &p_allowjump, 0 },
    { &p_usecontext, 0 },
    { &r_wipe, 1 },
    { &r_texnonpowresize, 0 },
    { &i_interpolateframes, 0 },
    { &r_texturecombiner, 1 },
    { &r_rendersprites, 1 },
    { &r_skybox, 0 },
    { &r_colorscale, 0 },
    { &am_showkeymarkers, 0 },
    { &am_showkeycolors, 0 },
    { &am_drawobjects, 0 },
    { &am_overlay, 0 },
    { &compat_collision, 1 },
    { &compat_limitpain, 1 },
    { &compat_mobjpass, 1 },
    { &compat_grabitems, 1 },
    { NULL, -1 }
};

menuthermobar_t SetupBars[] = {
    { misc_empty1, 80, &m_menufadetime },
    { misc_empty2, 50, &m_cursorscale },
    { -1, 0 }
};

menu_t MiscDef = {
    misc_end,
    false,
    &OptionsDef,
    MiscMenu,
    M_DrawMisc,
    "Setup",
    216,108,
    0,
    false,
    MiscDefault,
    14,
    0,
    0.5f,
    MiscHints,
    SetupBars
};

void M_Misc(int choice) {
    M_SetupNextMenu(&MiscDef);
}

void M_MiscChoice(int choice) {
    switch(itemOn) {
    case misc_menufade:
        if(choice) {
            if(m_menufadetime < 80.0f) {
                M_SetCvar(m_menufadetime, m_menufadetime + 0.8f);
            }
            else {
                m_menufadetime = 80.0f;
            }
        }
        else {
            if(m_menufadetime > 0.0f) {
                M_SetCvar(m_menufadetime, m_menufadetime - 0.8f);
            }
            else {
                m_menufadetime = 0.0f;
            }
        }
        break;

    case misc_cursorsize:
        if(choice) {
            if(m_cursorscale < 50.0f) {
                M_SetCvar(m_cursorscale, m_cursorscale + 0.5f);
            }
            else {
                m_cursorscale = 50.0f;
            }
        }
        else {
            if(m_cursorscale > 0.0f) {
                M_SetCvar(m_cursorscale, m_cursorscale - 0.5f);
            }
            else {
                m_cursorscale = 0;
            }
        }
        break;

    case misc_menumouse:
        M_SetOptionValue(choice, m_menumouse);
        break;

    case misc_aim:
        M_SetOptionValue(choice, p_autoaim);
        break;

    case misc_jump:
        M_SetOptionValue(choice, p_allowjump);
        break;

    case misc_context:
        M_SetOptionValue(choice, p_usecontext);
        break;

    case misc_wipe:
        M_SetOptionValue(choice, r_wipe);
        break;

    case misc_texresize:
        M_SetOptionValue(choice, 0, 2, 1, r_texnonpowresize);
        break;

    case misc_frame:
        M_SetOptionValue(choice, i_interpolateframes);
        break;

    case misc_combine:
        M_SetOptionValue(choice, r_texturecombiner);
        break;

    case misc_sprites:
        M_SetOptionValue(choice, 1, 2, 1, r_rendersprites);
        break;

    case misc_skybox:
        M_SetOptionValue(choice, r_skybox);
        break;

    case misc_rgbscale:
        M_SetOptionValue(choice, 0, 2, 1, r_colorscale);
        break;

    case misc_showkey:
        M_SetOptionValue(choice, am_showkeymarkers);
        break;

    case misc_showlocks:
        M_SetOptionValue(choice, am_showkeycolors);
        break;

    case misc_amobjects:
        M_SetOptionValue(choice, 0, 2, 1, am_drawobjects);
        break;

    case misc_amoverlay:
        M_SetOptionValue(choice, am_overlay);
        break;

    case misc_comp_collision:
        M_SetOptionValue(choice, compat_collision);
        break;

    case misc_comp_pain:
        M_SetOptionValue(choice, compat_limitpain);
        break;

    case misc_comp_pass:
        M_SetOptionValue(choice, compat_mobjpass);
        break;

    case misc_comp_grab:
        M_SetOptionValue(choice, compat_grabitems);
        break;
    }
}

void M_DrawMisc(void) {
    static const char* frametype[2] = { "Capped", "Smooth" };
    static const char* mapdisplaytype[2] = { "Hide", "Show" };
    static const char* objectdrawtype[3] = { "Arrows", "Sprites", "Both" };
    static const char* texresizetype[3] = { "Auto", "Padded", "Scaled" };
    static const char* rgbscaletype[3] = { "1x", "2x", "4x" };
    int y;

    if(currentMenu->menupageoffset <= misc_menufade+1 &&
            (misc_menufade+1) - currentMenu->menupageoffset < currentMenu->numpageitems) {
        y = misc_menufade - currentMenu->menupageoffset;
        M_DrawThermo(MiscDef.x,MiscDef.y+LINEHEIGHT*(y+1), 80, *m_menufadetime);
    }

    if(currentMenu->menupageoffset <= misc_cursorsize+1 &&
            (misc_cursorsize+1) - currentMenu->menupageoffset < currentMenu->numpageitems) {
        y = misc_cursorsize - currentMenu->menupageoffset;
        M_DrawThermo(MiscDef.x,MiscDef.y+LINEHEIGHT*(y+1), 50, *m_cursorscale);
    }

#define DRAWMISCITEM(a, b, c) \
    if(currentMenu->menupageoffset <= a && \
        a - currentMenu->menupageoffset < currentMenu->numpageitems) \
    { \
        y = a - currentMenu->menupageoffset; \
        Draw_BigText(MiscDef.x + 176, MiscDef.y+LINEHEIGHT*y, MENUCOLORRED, \
            c[(int)b]); \
    }

    DRAWMISCITEM(misc_menumouse, *m_menumouse, msgNames);
    DRAWMISCITEM(misc_aim, *p_autoaim, msgNames);
    DRAWMISCITEM(misc_jump, *p_allowjump, msgNames);
    DRAWMISCITEM(misc_context, *p_usecontext, mapdisplaytype);
    DRAWMISCITEM(misc_wipe, *r_wipe, msgNames);
    DRAWMISCITEM(misc_texresize, *r_texnonpowresize, texresizetype);
    DRAWMISCITEM(misc_frame, *i_interpolateframes, frametype);
    DRAWMISCITEM(misc_combine, *r_texturecombiner, msgNames);
    DRAWMISCITEM(misc_sprites, *r_rendersprites - 1, msgNames);
    DRAWMISCITEM(misc_skybox, *r_skybox, msgNames);
    DRAWMISCITEM(misc_rgbscale, *r_colorscale, rgbscaletype);
    DRAWMISCITEM(misc_showkey, *am_showkeymarkers, mapdisplaytype);
    DRAWMISCITEM(misc_showlocks, *am_showkeycolors, mapdisplaytype);
    DRAWMISCITEM(misc_amobjects, *am_drawobjects, objectdrawtype);
    DRAWMISCITEM(misc_amoverlay, *am_overlay, msgNames);
    DRAWMISCITEM(misc_comp_collision, *compat_collision, msgNames);
    DRAWMISCITEM(misc_comp_pain, *compat_limitpain, msgNames);
    DRAWMISCITEM(misc_comp_pass, !*compat_mobjpass, msgNames);
    DRAWMISCITEM(misc_comp_grab, *compat_grabitems, msgNames);

#undef DRAWMISCITEM

    if(MiscDef.hints[itemOn] != NULL) {
        GL_SetOrthoScale(0.5f);
        Draw_BigText(-1, 410, MENUCOLORWHITE, MiscDef.hints[itemOn]);
        GL_SetOrthoScale(MiscDef.scale);
    }
}

//------------------------------------------------------------------------
//
// MOUSE MENU
//
//------------------------------------------------------------------------

void M_ChangeSensitivity(int choice);
void M_ChangeMouseAccel(int choice);
void M_ChangeMouseLook(int choice);
void M_ChangeMouseInvert(int choice);
void M_ChangeYAxisMove(int choice);
void M_DrawMouse(void);

extern FloatProperty v_msensitivityx;
extern FloatProperty v_msensitivityy;
extern BoolProperty v_mlook;
extern BoolProperty v_mlookinvert;
extern BoolProperty v_yaxismove;
extern FloatProperty v_macceleration;

enum {
    mouse_sensx,
    mouse_empty1,
    mouse_sensy,
    mouse_empty2,
    mouse_accel,
    mouse_empty3,
    mouse_look,
    mouse_invert,
    mouse_yaxismove,
    mouse_default,
    mouse_return,
    mouse_end
};

menuitem_t MouseMenu[]= {
    {3,"Mouse Sensitivity X",M_ChangeSensitivity, 'x'},
    {-1,"",0},
    {3,"Mouse Sensitivity Y",M_ChangeSensitivity, 'y'},
    {-1,"",0},
    {3, "Mouse Acceleration",M_ChangeMouseAccel, 'a'},
    {-1, "",0},
    {2,"Mouse Look:",M_ChangeMouseLook,'l'},
    {2,"Invert Look:",M_ChangeMouseInvert, 'i'},
    {2,"Y-Axis Move:",M_ChangeYAxisMove, 'y'},
    {-2,"Default",M_DoDefaults,'d'},
    {1,"/r Return",M_Return, 0x20}
};

menudefault_t MouseDefault[] = {
    { &v_msensitivityx, 5 },
    { &v_msensitivityy, 5 },
    { &v_macceleration, 0 },
    { &v_mlook, 0 },
    { &v_mlookinvert, 0 },
    { &v_yaxismove, 0 },
    { NULL, -1 }
};

menuthermobar_t MouseBars[] = {
    { mouse_empty1, 32, &v_msensitivityx },
    { mouse_empty2, 32, &v_msensitivityy },
    { mouse_empty3, 20, &v_macceleration },
    { -1, 0 }
};

menu_t MouseDef = {
    mouse_end,
    false,
    &ControlMenuDef,
    MouseMenu,
    M_DrawMouse,
    "Mouse",
    104,52,
    0,
    false,
    MouseDefault,
    -1,
    0,
    0.925f,
    NULL,
    MouseBars
};

void M_DrawMouse(void) {
    M_DrawThermo(MouseDef.x,MouseDef.y+LINEHEIGHT*(mouse_sensx+1),MAXSENSITIVITY, *v_msensitivityx);
    M_DrawThermo(MouseDef.x,MouseDef.y+LINEHEIGHT*(mouse_sensy+1),MAXSENSITIVITY, *v_msensitivityy);

    M_DrawThermo(MouseDef.x,MouseDef.y+LINEHEIGHT*(mouse_accel+1),20, *v_macceleration);

    Draw_BigText(MouseDef.x + 144, MouseDef.y+LINEHEIGHT*mouse_look, MENUCOLORRED,
                 msgNames[*v_mlook]);
    Draw_BigText(MouseDef.x + 144, MouseDef.y+LINEHEIGHT*mouse_invert, MENUCOLORRED,
                 msgNames[*v_mlookinvert]);
    Draw_BigText(MouseDef.x + 144, MouseDef.y+LINEHEIGHT*mouse_yaxismove, MENUCOLORRED,
                 msgNames[*v_yaxismove]);
}

void M_ChangeSensitivity(int choice) {
    float slope = (float)MAXSENSITIVITY / 100.0f;
    switch(choice) {
    case 0:
        switch(itemOn) {
        case mouse_sensx:
            if(v_msensitivityx > 0.0f) {
                M_SetCvar(v_msensitivityx, v_msensitivityx - slope);
            }
            else {
                v_msensitivityx = 0.0f;
            }
            break;
        case mouse_sensy:
            if(v_msensitivityy > 0.0f) {
                M_SetCvar(v_msensitivityy, v_msensitivityy - slope);
            }
            else {
                v_msensitivityy = 0.0f;
            }
            break;
        }
        break;
    case 1:
        switch(itemOn) {
        case mouse_sensx:
            if(v_msensitivityx < (float)MAXSENSITIVITY) {
                M_SetCvar(v_msensitivityx, v_msensitivityx + slope);
            }
            else {
                v_msensitivityx = (float)MAXSENSITIVITY;
            }
            break;
        case mouse_sensy:
            if(v_msensitivityy < (float)MAXSENSITIVITY) {
                M_SetCvar(v_msensitivityy, v_msensitivityy + slope);
            }
            else {
                v_msensitivityy = (float)MAXSENSITIVITY;
            }
            break;
        }
        break;
    }
}

void M_ChangeMouseAccel(int choice) {
    float slope = 20.0f / 100.0f;
    switch(choice) {
    case 0:
        if(v_macceleration > 0.0f) {
            M_SetCvar(v_macceleration, v_macceleration - slope);
        }
        else {
            v_macceleration = 0;
        }
        break;
    case 1:
        if(v_macceleration < 20.0f) {
            M_SetCvar(v_macceleration, v_macceleration + slope);
        }
        else {
            v_macceleration = 20.0f;
        }
        break;
    }
}

void M_ChangeMouseLook(int choice) {
    M_SetOptionValue(choice, v_mlook);
}

void M_ChangeMouseInvert(int choice) {
    M_SetOptionValue(choice, v_mlookinvert);
}

void M_ChangeYAxisMove(int choice) {
    M_SetOptionValue(choice, v_yaxismove);
}

//------------------------------------------------------------------------
//
// DISPLAY MENU
//
//------------------------------------------------------------------------

void M_ChangeBrightness(int choice);
void M_ChangeMessages(int choice);
void M_ToggleHudDraw(int choice);
void M_ToggleFlashOverlay(int choice);
void M_ToggleDamageHud(int choice);
void M_ToggleWpnDisplay(int choice);
void M_ToggleShowStats(int choice);
void M_ChangeCrosshair(int choice);
void M_ChangeOpacity(int choice);
void M_DrawDisplay(void);

extern IntProperty st_drawhud;
extern BoolProperty st_crosshair;
extern FloatProperty st_crosshairopacity;
extern BoolProperty st_flashoverlay;
extern BoolProperty st_showpendingweapon;
extern BoolProperty st_showstats;
extern FloatProperty i_brightness;
extern BoolProperty m_messages;
extern BoolProperty p_damageindicator;

enum {
    dbrightness,
    display_empty1,
    messages,
    statusbar,
    display_flash,
    display_damage,
    display_weapon,
    display_stats,
    display_crosshair,
    display_opacity,
    display_empty2,
    e_default,
    display_return,
    display_end
};

menuitem_t DisplayMenu[]= {
    {3,"Brightness",M_ChangeBrightness, 'b'},
    {-1,"",0},
    {2,"Messages:",M_ChangeMessages, 'm'},
    {2,"Status Bar:",M_ToggleHudDraw, 's'},
    {2,"Hud Flash:",M_ToggleFlashOverlay, 'f'},
    {2,"Damage Hud:",M_ToggleDamageHud, 'd'},
    {2,"Show Weapon:",M_ToggleWpnDisplay, 'w'},
    {2,"Show Stats:",M_ToggleShowStats, 't'},
    {2,"Crosshair:",M_ChangeCrosshair, 'c'},
    {3,"Crosshair Opacity",M_ChangeOpacity, 'o'},
    {-1,"",0},
    {-2,"Default",M_DoDefaults, 'd'},
    {1,"/r Return",M_Return, 0x20}
};

const char* DisplayHints[display_end]= {
    "change light color intensity",
    NULL,
    "toggle messages displaying on hud",
    "change look and style for hud",
    "use texture environment or a simple overlay for flashes",
    "toggle hud indicators when taking damage",
    "shows the next or previous pending weapon",
    "display level stats in automap",
    "toggle crosshair",
    "change opacity for crosshairs",
    NULL,
    NULL,
    NULL
};

menudefault_t DisplayDefault[] = {
    { &i_brightness, 0 },
    { &m_messages, 1 },
    { &st_drawhud, 1 },
    { &p_damageindicator, 0 },
    { &st_showpendingweapon, 1 },
    { &st_showstats, 0 },
    { &st_crosshair, 0 },
    { &st_crosshairopacity, 80 },
    { NULL, -1 }
};

menuthermobar_t DisplayBars[] = {
    { display_empty1, 100, &i_brightness },
    { display_empty2, 255, &st_crosshairopacity },
    { -1, 0 }
};

menu_t DisplayDef = {
    display_end,
    false,
    &OptionsDef,
    DisplayMenu,
    M_DrawDisplay,
    "Display",
    165,65,
    0,
    false,
    DisplayDefault,
    -1,
    0,
    0.715f,
    DisplayHints,
    DisplayBars
};

void M_Display(int choice) {
    M_SetupNextMenu(&DisplayDef);
}

void M_DrawDisplay(void) {
    static const char* hudtype[3] = { "Off", "Classic", "Arranged" };
    static const char* flashtype[2] = { "Environment", "Overlay" };

    M_DrawThermo(DisplayDef.x, DisplayDef.y+LINEHEIGHT*(dbrightness+1), MAXBRIGHTNESS, *i_brightness);
    Draw_BigText(DisplayDef.x + 140, DisplayDef.y+LINEHEIGHT*messages, MENUCOLORRED,
                 msgNames[*m_messages]);
    Draw_BigText(DisplayDef.x + 140, DisplayDef.y+LINEHEIGHT*statusbar, MENUCOLORRED,
                 hudtype[*st_drawhud]);
    Draw_BigText(DisplayDef.x + 140, DisplayDef.y+LINEHEIGHT*display_flash, MENUCOLORRED,
                 flashtype[*st_flashoverlay]);
    Draw_BigText(DisplayDef.x + 140, DisplayDef.y+LINEHEIGHT*display_damage, MENUCOLORRED,
                 msgNames[*p_damageindicator]);
    Draw_BigText(DisplayDef.x + 140, DisplayDef.y+LINEHEIGHT*display_weapon, MENUCOLORRED,
                 msgNames[*st_showpendingweapon]);
    Draw_BigText(DisplayDef.x + 140, DisplayDef.y+LINEHEIGHT*display_stats, MENUCOLORRED,
                 msgNames[*st_showstats]);

    if(st_crosshair <= 0) {
        Draw_BigText(DisplayDef.x + 140, DisplayDef.y+LINEHEIGHT*display_crosshair, MENUCOLORRED,
                     msgNames[0]);
    }
    else {
        ST_DrawCrosshair(DisplayDef.x + 140, DisplayDef.y+LINEHEIGHT*display_crosshair,
                         (int)st_crosshair, 1, MENUCOLORWHITE);
    }

    M_DrawThermo(DisplayDef.x, DisplayDef.y+LINEHEIGHT*(display_opacity+1),
                 255, *st_crosshairopacity);

    if(DisplayDef.hints[itemOn] != NULL) {
        GL_SetOrthoScale(0.5f);
        Draw_BigText(-1, 432, MENUCOLORWHITE, DisplayDef.hints[itemOn]);
        GL_SetOrthoScale(DisplayDef.scale);
    }
}

void M_ChangeBrightness(int choice) {
    switch(choice) {
    case 0:
        if(i_brightness > 0.0f) {
            M_SetCvar(i_brightness, i_brightness - 1);
        }
        else {
            i_brightness = 0;
        }
        break;
    case 1:
        if(i_brightness < (int)MAXBRIGHTNESS) {
            M_SetCvar(i_brightness, i_brightness + 1);
        }
        else {
            i_brightness = (int)MAXBRIGHTNESS;
        }
        break;
    }

    R_RefreshBrightness();
}

void M_ChangeMessages(int choice) {
    M_SetOptionValue(choice, m_messages);

    if(choice) {
        players[consoleplayer].message = MSGON;
    }
    else {
        players[consoleplayer].message = MSGOFF;
    }
}

void M_ToggleHudDraw(int choice) {
    M_SetOptionValue(choice, 0, 2, 1, st_drawhud);
}

void M_ToggleDamageHud(int choice) {
    M_SetOptionValue(choice, p_damageindicator);
}

void M_ToggleWpnDisplay(int choice) {
    M_SetOptionValue(choice, st_showpendingweapon);
}

void M_ToggleShowStats(int choice) {
    M_SetOptionValue(choice, st_showstats);
}

void M_ToggleFlashOverlay(int choice) {
    M_SetOptionValue(choice, st_flashoverlay);
}

void M_ChangeCrosshair(int choice) {
    M_SetOptionValue(choice, 0, st_crosshairs, 1, st_crosshair);
}

void M_ChangeOpacity(int choice) {
    float slope = 255.0f / 100.0f;

    if(choice) {
        if(st_crosshairopacity < 255.0f) {
            M_SetCvar(st_crosshairopacity, st_crosshairopacity + slope);
        }
        else {
            st_crosshairopacity = 255.0f;
        }
    }
    else {
        if(st_crosshairopacity > 0.0f) {
            M_SetCvar(st_crosshairopacity, st_crosshairopacity - slope);
        }
        else {
            st_crosshairopacity = 0.0f;
        }
    }
}

//------------------------------------------------------------------------
//
// VIDEO MENU
//
//------------------------------------------------------------------------

void M_ChangeGammaLevel(int choice);
void M_ChangeFilter(int choice);
void M_ChangeWindowed(int choice);
void M_ChangeRatio(int choice);
void M_ChangeResolution(int choice);
void M_ChangeVSync(int choice);
void M_ChangeDepthSize(int choice);
void M_ChangeBufferSize(int choice);
void M_ChangeAnisotropic(int choice);
void M_DrawVideo(void);

extern IntProperty v_width;
extern IntProperty v_height;
extern IntProperty v_windowed;
extern BoolProperty v_vsync;
extern IntProperty v_depthsize;
extern IntProperty v_buffersize;
extern FloatProperty i_gamma;
extern FloatProperty i_brightness;
extern BoolProperty r_filter;
extern BoolProperty r_anisotropic;

enum {
    video_dgamma,
    video_empty1,
    filter,
    anisotropic,
    windowed,
    vsync,
    depth,
    buffer,
    resolution,
    v_default,
    video_return,
    video_end
};

menuitem_t VideoMenu[]= {
    {3,"Gamma Correction",M_ChangeGammaLevel, 'g'},
    {-1,"",0},
    {2,"Filter:",M_ChangeFilter, 'f'},
    {2,"Anisotropy:",M_ChangeAnisotropic, 'a'},
    {2,"Windowed:",M_ChangeWindowed, 'w'},
    {2,"Vsync:",M_ChangeVSync, 'v'},
    {2,"Depth Size:",M_ChangeDepthSize, 'd'},
    {2,"Buffer Size:",M_ChangeBufferSize, 'b'},
    {2,"Resolution:",M_ChangeResolution, 'r'},
    {-2,"Default",M_DoDefaults, 'e'},
    {1,"/r Return",M_Return, 0x20}
};

menudefault_t VideoDefault[] = {
    { &i_gamma, 0 },
    { &r_filter, 0 },
    { &r_anisotropic, 0 },
    { &v_windowed, 1 },
    { &v_vsync, 1 },
    { &v_depthsize, 24 },
    { &v_buffersize, 32 },
    { NULL, -1 },
    { NULL, -1 }
};

menuthermobar_t VideoBars[] = {
    { video_empty1, 20, &i_gamma },
    { -1, 0 }
};

menu_t VideoDef = {
    video_end,
    false,
    &OptionsDef,
    VideoMenu,
    M_DrawVideo,
    "Video",
    136,80,
    0,
    false,
    VideoDefault,
    12,
    0,
    0.65f,
    NULL,
    VideoBars
};

static char gammamsg[21][28] = {
    GAMMALVL0,
    GAMMALVL1,
    GAMMALVL2,
    GAMMALVL3,
    GAMMALVL4,
    GAMMALVL5,
    GAMMALVL6,
    GAMMALVL7,
    GAMMALVL8,
    GAMMALVL9,
    GAMMALVL10,
    GAMMALVL11,
    GAMMALVL12,
    GAMMALVL13,
    GAMMALVL14,
    GAMMALVL15,
    GAMMALVL16,
    GAMMALVL17,
    GAMMALVL18,
    GAMMALVL19,
    GAMMALVL20
};

void M_Video(int choice) {
    int i;

    M_SetupNextMenu(&VideoDef);

    m_ScreenSize = -1;
}

void M_DrawVideo(void) {
    static const char* filterType[2] = { "Linear", "Nearest" };
    static const char* windowType[3] = { "Off", "On", "Noborder" };
    static char bitValue[8];
    char res[16];
    int y;

    if(currentMenu->menupageoffset <= video_dgamma + 1 &&
            (video_dgamma+1) - currentMenu->menupageoffset < currentMenu->numpageitems) {
        y = video_dgamma - currentMenu->menupageoffset;
        M_DrawThermo(VideoDef.x, VideoDef.y + LINEHEIGHT*(y + 1), 20, *i_gamma);
    }

#define DRAWVIDEOITEM(a, b) \
    if(currentMenu->menupageoffset <= a && \
        a - currentMenu->menupageoffset < currentMenu->numpageitems) \
    { \
        y = a - currentMenu->menupageoffset; \
        Draw_BigText(VideoDef.x + 176, VideoDef.y+LINEHEIGHT*y, MENUCOLORRED, b); \
    }

#define DRAWVIDEOITEM2(a, b, c) DRAWVIDEOITEM(a, c[(int)b])

    DRAWVIDEOITEM2(filter, *r_filter, filterType);
    DRAWVIDEOITEM2(anisotropic, *r_anisotropic, msgNames);
    DRAWVIDEOITEM2(windowed, *v_windowed, windowType);

    if (m_ScreenSize < 0) {
        sprintf(res, "%ix%i", (int) v_width, (int) v_height);
    } else {
        auto mode = Video->modes()[m_ScreenSize];
        sprintf(res, "%ix%i", mode.width, mode.height);
    }
    DRAWVIDEOITEM(resolution, res);

    DRAWVIDEOITEM2(vsync, *v_vsync, msgNames);

    if(currentMenu->menupageoffset <= depth &&
            depth - currentMenu->menupageoffset < currentMenu->numpageitems) {
        if(v_depthsize == 8) {
            dsnprintf(bitValue, 2, "8");
        }
        else if(v_depthsize == 16) {
            dsnprintf(bitValue, 3, "16");
        }
        else if(v_depthsize == 24) {
            dsnprintf(bitValue, 3, "24");
        }
        else {
            dsnprintf(bitValue, 8, "Invalid");
        }

        y = depth - currentMenu->menupageoffset;
        Draw_BigText(VideoDef.x + 176, VideoDef.y+LINEHEIGHT*y, MENUCOLORRED, bitValue);
    }

    if(currentMenu->menupageoffset <= buffer &&
            buffer - currentMenu->menupageoffset < currentMenu->numpageitems) {
        if(v_buffersize == 8) {
            dsnprintf(bitValue, 2, "8");
        }
        else if(v_buffersize == 16) {
            dsnprintf(bitValue, 3, "16");
        }
        else if(v_buffersize == 24) {
            dsnprintf(bitValue, 3, "24");
        }
        else if(v_buffersize == 32) {
            dsnprintf(bitValue, 3, "32");
        }
        else {
            dsnprintf(bitValue, 8, "Invalid");
        }

        y = buffer - currentMenu->menupageoffset;
        Draw_BigText(VideoDef.x + 176, VideoDef.y+LINEHEIGHT*y, MENUCOLORRED, bitValue);
    }

#undef DRAWVIDEOITEM
#undef DRAWVIDEOITEM2

    Draw_Text(145, 308, MENUCOLORWHITE, VideoDef.scale, false,
              "Changes will take effect\n when exiting this menu.");

    GL_SetOrthoScale(VideoDef.scale);
}

void M_VideoSetMode()
{
    VideoMode mode {};
    mode.width = *v_width;
    mode.height = *v_height;
    mode.vsync = *v_vsync;
    mode.buffer_size = *v_buffersize;
    mode.depth_size = *v_depthsize;

    switch (*v_windowed) {
    case 0:
        mode.fullscreen = Fullscreen::exclusive;
        break;

    case 2:
        mode.fullscreen = Fullscreen::noborder;
        break;

    default:
        mode.fullscreen = Fullscreen::none;
        break;
    }

    Video->set_mode(mode);
}

void M_ChangeGammaLevel(int choice) {
    float slope = 20.0f / 100.0f;
    switch(choice) {
    case 0:
        if(i_gamma > 0.0f) {
            M_SetCvar(i_gamma, i_gamma - slope);
        }
        else {
            i_gamma = 0;
        }
        break;
    case 1:
        if(i_gamma < 20.0f) {
            M_SetCvar(i_gamma, i_gamma + slope);
        }
        else {
            i_gamma = 20.0f;
        }
        break;
    case 2:
        if(i_gamma >= 20) {
            i_gamma = 0.0f;
        }
        else {
            i_gamma = i_gamma + 1;
        }

        players[consoleplayer].message = gammamsg[(int)i_gamma];
        break;
    }
}

void M_ChangeFilter(int choice) {
    M_SetOptionValue(choice, r_filter);
}

void M_ChangeAnisotropic(int choice) {
    M_SetOptionValue(choice, r_anisotropic);
}

void M_ChangeWindowed(int choice) {
    M_SetOptionValue(choice, 0, 2, 1, v_windowed);
}

static void M_SetResolution(void) {
    if (m_ScreenSize < 0)
        return;

    auto mode = Video->modes()[m_ScreenSize];
    M_SetCvar(v_width, mode.width);
    M_SetCvar(v_height, mode.height);
}

void M_ChangeResolution(int choice) {
    int max = Video->modes().size();

    if(choice) {
        if(++m_ScreenSize > max - 1) {
            if(choice == 2) {
                m_ScreenSize = 0;
            }
            else {
                m_ScreenSize = max - 1;
            }
        }
    }
    else {
        m_ScreenSize = MAX(m_ScreenSize - 1, 0);
    }

    M_SetResolution();
}

void M_ChangeVSync(int choice) {
    M_SetOptionValue(choice, v_vsync);
}

void M_ChangeDepthSize(int choice) {
    M_SetOptionValue(choice, 8, 24, 8, v_depthsize);
}

void M_ChangeBufferSize(int choice) {
    M_SetOptionValue(choice, 8, 32, 8, v_buffersize);
}

//------------------------------------------------------------------------
//
// PASSWORD MENU
//
//------------------------------------------------------------------------

void M_DrawPassword(void);

menuitem_t PasswordMenu[32];

menu_t PasswordDef = {
    32,
    false,
    &OptionsDef,
    PasswordMenu,
    M_DrawPassword,
    "Password",
    92,60,
    0,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

static dboolean passInvalid = false;
static int        curPasswordSlot = 0;
static int        passInvalidTic = 0;

void M_Password(int choice) {
    M_SetupNextMenu(&PasswordDef);
    passInvalid = false;
    passInvalidTic = 0;

    for(curPasswordSlot = 0; curPasswordSlot < 15; curPasswordSlot++) {
        if(passwordData[curPasswordSlot] == 0xff) {
            break;
        }
    }
}

void M_DrawPassword(void) {
    char password[2];
    byte *passData;
    int i = 0;

#ifdef _USE_XINPUT  // XINPUT
    if(!xgamepad.connected)
#endif
    {
        Draw_BigText(-1, 240 - 48, MENUCOLORWHITE , "Press Delete To Change");
        Draw_BigText(-1, 240 - 32, MENUCOLORWHITE , "Press Escape To Return");
    }

    if(passInvalid) {
        if(!passInvalidTic--) {
            passInvalidTic = 0;
            passInvalid = false;
        }

        if(passInvalidTic & 16) {
            Draw_BigText(-1, 240 - 80, MENUCOLORWHITE, "Invalid Password");
            return;
        }
    }

    dmemset(password, 0, 2);
    passData = passwordData;

    for(i = 0; i < 19; i++) {
        if(i == 4 || i == 9 || i == 14) {
            password[0] = 0x20;
        }
        else {
            if(*passData == 0xff) {
                password[0] = '.';
            }
            else {
                password[0] = passwordChar[*passData];
            }

            passData++;
        }

        Draw_BigText((currentMenu->x + (i * 12)) - 48, 240 - 80, MENUCOLORRED, password);
    }
}

static void M_PasswordSelect(void) {
    S_StartSound(NULL, sfx_switch2);
    passwordData[curPasswordSlot++] = (byte)itemOn;
    if(curPasswordSlot > 15) {
        static const char* hecticdemo = "rvnh3ct1cd3m0???";
        int i;

        for(i = 0; i < 16; i++) {
            if(passwordChar[passwordData[i]] != hecticdemo[i]) {
                break;
            }
        }

        if(i >= 16) {
            rundemo4 = true;
            M_Return(0);

            return;
        }

        if(!M_DecodePassword(1)) {
            passInvalid = true;
            passInvalidTic = 64;
        }
        else {
            M_DecodePassword(0);
            G_DeferedInitNew(gameskill, gamemap);
            doPassword = true;
            currentMenu->lastOn = itemOn;
            S_StartSound(NULL, sfx_pistol);
            M_ClearMenus();

            return;
        }

        curPasswordSlot = 15;
    }
}

static void M_PasswordDeSelect(void) {
    S_StartSound(NULL, sfx_switch2);
    if(passwordData[curPasswordSlot] == 0xff) {
        curPasswordSlot--;
    }

    if(curPasswordSlot < 0) {
        curPasswordSlot = 0;
    }

    passwordData[curPasswordSlot] = 0xff;
}

//------------------------------------------------------------------------
//
// SOUND MENU
//
//------------------------------------------------------------------------

void M_SfxVol(int choice);
void M_MusicVol(int choice);
void M_GainOutput(int choice);
void M_DrawSound(void);

extern FloatProperty s_sfxvol;
extern FloatProperty s_musvol;
extern FloatProperty s_gain;

enum {
    sfx_vol,
    sfx_empty1,
    music_vol,
    sfx_empty2,
    gain,
    sfx_empty3,
    sound_default,
    sound_return,
    sound_end
};

menuitem_t SoundMenu[]= {
    {3,"Sound Volume",M_SfxVol,'s'},
    {-1,"",0},
    {3,"Music Volume",M_MusicVol,'m'},
    {-1,"",0},
    {3,"Gain Output",M_GainOutput,'g'},
    {-1,"",0},
    {-2,"Default",M_DoDefaults,'d'},
    {1,"/r Return",M_Return, 0x20}
};

menudefault_t SoundDefault[] = {
    { &s_sfxvol, 80 },
    { &s_musvol, 80 },
    { &s_gain, 1 },
    { NULL, -1 }
};

menuthermobar_t SoundBars[] = {
    { sfx_empty1, 100, &s_sfxvol },
    { sfx_empty2, 100, &s_musvol },
    { sfx_empty3, 2, &s_gain },
    { -1, 0 }
};

menu_t SoundDef = {
    sound_end,
    false,
    &OptionsDef,
    SoundMenu,
    M_DrawSound,
    "Sound",
    96,60,
    0,
    false,
    SoundDefault,
    -1,
    0,
    1.0f,
    NULL,
    SoundBars
};

void M_Sound(int choice) {
    M_SetupNextMenu(&SoundDef);
}

void M_DrawSound(void) {
    M_DrawThermo(SoundDef.x,SoundDef.y+LINEHEIGHT*(sfx_vol+1), 100, *s_sfxvol);
    M_DrawThermo(SoundDef.x,SoundDef.y+LINEHEIGHT*(music_vol+1), 100, *s_musvol);
    M_DrawThermo(SoundDef.x,SoundDef.y+LINEHEIGHT*(gain+1), 2, *s_gain);
}

void M_SfxVol(int choice) {
    float slope = 1.0f;
    switch(choice) {
    case 0:
        if(s_sfxvol > 0.0f) {
            M_SetCvar(s_sfxvol, s_sfxvol - slope);
        }
        else {
            s_sfxvol = 0.0f;
        }
        break;
    case 1:
        if(s_sfxvol < 100.0f) {
            M_SetCvar(s_sfxvol, s_sfxvol + slope);
        }
        else {
            s_sfxvol = 100.0f;
        }
        break;
    }
}

void M_MusicVol(int choice) {
    float slope = 1.0f;
    switch(choice) {
    case 0:
        if(s_musvol > 0.0f) {
            M_SetCvar(s_musvol, s_musvol - slope);
        }
        else {
            s_musvol = 0.0f;
        }
        break;
    case 1:
        if(s_musvol < 100.0f) {
            M_SetCvar(s_musvol, s_musvol + slope);
        }
        else {
            s_musvol = 100.0f;
        }
        break;
    }
}

void M_GainOutput(int choice) {
    float slope = 2.0f / 100.0f;
    switch(choice) {
    case 0:
        if(s_gain > 0.0f) {
            M_SetCvar(s_gain, s_gain - slope);
        }
        else {
            s_gain = 0.0f;
        }
        break;
    case 1:
        if(s_gain < 2.0f) {
            M_SetCvar(s_gain, s_gain + slope);
        }
        else {
            s_gain = 2.0f;
        }
        break;
    }
}

//------------------------------------------------------------------------
//
// FEATURES MENU
//
//------------------------------------------------------------------------

void M_DoFeature(int choice);
void M_DrawFeaturesMenu(void);

extern BoolProperty sv_lockmonsters;

enum {
    features_levels = 0,
    features_invulnerable,
    features_healthboost,
    features_weapons,
    features_mapeverything,
    features_securitykeys,
    features_lockmonsters,
    features_noclip,
    features_wireframe,
    features_end
};

#define FEATURESWARPLEVEL    "Warp To Level:"
#define FEATURESWARPFUN        "Warp To Fun:"
#define FEATURESWARPSINGLE  "Warp To Pwad:"

menuitem_t FeaturesMenu[]= {
    {2,FEATURESWARPLEVEL,M_DoFeature,'l'},
    {2,"Invulnerable:",M_DoFeature,'i'},
    {2,"Health Boost:",M_DoFeature,'h'},
    {2,"Weapons:",M_DoFeature,'w'},
    {2,"Map Everything:",M_DoFeature,'m'},
    {2,"Security Keys:",M_DoFeature,'k'},
    {2,"Lock Monsters:",M_DoFeature,'o'},
    {2,"Wall Blocking:",M_DoFeature,'w'},
    {2,"Wireframe Mode:",M_DoFeature,'r'},
};

menu_t featuresDef = {
    features_end,
    false,
    &PauseDef,
    FeaturesMenu,
    M_DrawFeaturesMenu,
    "Features",
    56,56,
    0,
    true,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

void M_Features(int choice) {
    M_SetupNextMenu(&featuresDef);

    showfullitemvalue[0]=showfullitemvalue[1]=showfullitemvalue[2]=false;
}

void M_DrawFeaturesMenu(void) {
    mapdef_t* map = P_GetMapInfo(levelwarp + 1);

    /*Warp To Level*/
    M_DrawSmbString(map->mapname, &featuresDef, features_levels);

    /*Lock Monsters Mode*/
    M_DrawSmbString(msgNames[(int)sv_lockmonsters], &featuresDef, features_lockmonsters);

    /*Wireframe Mode*/
    M_DrawSmbString(msgNames[wireframeon], &featuresDef, features_wireframe);

    /*Invulnerable*/
    M_DrawSmbString(msgNames[players[consoleplayer].cheats & CF_GODMODE ? 1 : 0],
                    &featuresDef, features_invulnerable);

    /*No Clip*/
    M_DrawSmbString(msgNames[players[consoleplayer].cheats & CF_NOCLIP ? 1 : 0],
                    &featuresDef, features_noclip);

    /*Map Everything*/
    M_DrawSmbString(msgNames[amCheating==2 ? 1 : 0], &featuresDef, features_mapeverything);

    /*Full Health*/
    M_DrawSmbString(showfullitemvalue[0] ? "100%%" : "-", &featuresDef, features_healthboost);

    /*Full Weapons*/
    M_DrawSmbString(showfullitemvalue[1] ? "100%%" : "-", &featuresDef, features_weapons);

    /*Full Keys*/
    M_DrawSmbString(showfullitemvalue[2] ? "100%%" : "-", &featuresDef, features_securitykeys);

    switch(map->type) {
    case 0:
        sprintf(featuresDef.menuitems[features_levels].name, FEATURESWARPLEVEL);
        break;
    case 1:
        sprintf(featuresDef.menuitems[features_levels].name, FEATURESWARPFUN);
        break;
    case 2:
        sprintf(featuresDef.menuitems[features_levels].name, FEATURESWARPSINGLE);
        break;
    }
}

void M_DoFeature(int choice) {
    int i = 0;

    switch(itemOn) {
    case features_levels:
        if(choice) {
            levelwarp++;
            if(levelwarp >= 31) {
                if(choice == 2) {
                    levelwarp = 0;
                }
                else {
                    levelwarp = 31;
                }
            }
        }
        else {
            levelwarp--;
            if(levelwarp <= 0) {
                levelwarp = 0;
            }
        }
        break;

    case features_invulnerable:
        if(choice) {
            players[consoleplayer].cheats |= CF_GODMODE;
        }
        else {
            players[consoleplayer].cheats &= ~CF_GODMODE;
        }
        break;

    case features_noclip:
        if(choice) {
            players[consoleplayer].cheats |= CF_NOCLIP;
        }
        else {
            players[consoleplayer].cheats &= ~CF_NOCLIP;
        }
        break;

    case features_healthboost:
        showfullitemvalue[0] = true;
        players[consoleplayer].health = 100;
        players[consoleplayer].mo->health = 100;
        break;

    case features_weapons:
        showfullitemvalue[1] = true;

        for(i = 0; i < NUMWEAPONS; i++) {
            players[consoleplayer].weaponowned[i] = true;
        }

        if(!players[consoleplayer].backpack) {
            players[consoleplayer].backpack = true;
            for(i = 0; i < NUMAMMO; i++) {
                players[consoleplayer].maxammo[i] *= 2;
            }
        }

        for(i = 0; i < NUMAMMO; i++) {
            players[consoleplayer].ammo[i] = players[consoleplayer].maxammo[i];
        }

        break;

    case features_mapeverything:
        amCheating = choice ? 2 : 0;
        break;

    case features_securitykeys:
        showfullitemvalue[2] = true;

        for(i = 0; i < NUMCARDS; i++) {
            players[consoleplayer].cards[i] = true;
        }

        break;

    case features_lockmonsters:
        sv_lockmonsters = choice;
        break;

    case features_wireframe:
        R_DrawWireframe(choice);
        wireframeon = choice;
        break;
    }

    S_StartSound(NULL, sfx_switch2);
}

#ifdef _USE_XINPUT  // XINPUT

#include "g_controls.h"

//------------------------------------------------------------------------
//
// XBOX 360 CONTROLLER MENU
//
//------------------------------------------------------------------------

void M_XGamePadChoice(int choice);
void M_DrawXGamePad(void);

CVAR_EXTERNAL(i_rsticksensitivity);
CVAR_EXTERNAL(i_rstickthreshold);
CVAR_EXTERNAL(i_xinputscheme);

enum {
    xgp_sensitivity,
    xgp_empty1,
    xgp_threshold,
    xgp_empty2,
    xgp_look,
    xgp_invert,
    xgp_default,
    xgp_return,
    xgp_end
} xgp_e;

menuitem_t XGamePadMenu[]= {
    {3,"Stick Sensitivity",M_XGamePadChoice,'s'},
    {-1,"",0},
    {3,"Turn Threshold",M_XGamePadChoice,'t'},
    {-1,"",0},
    {2,"Y Axis Look:",M_ChangeMouseLook,'l'},
    {2,"Invert Look:",M_ChangeMouseInvert, 'i'},
    {-2,"Default",M_DoDefaults,'d'},
    {1,"/r Return",M_Return, 0x20}
};

menudefault_t XGamePadDefault[] = {
    { &i_rsticksensitivity, 0.0080f },
    { &i_rstickthreshold, 20 },
    { &v_mlook, 0 },
    { &v_mlookinvert, 0 },
    { NULL, -1 }
};

menu_t XGamePadDef = {
    xgp_end,
    false,
    &ControlMenuDef,
    XGamePadMenu,
    M_DrawXGamePad,
    "XBOX 360 Gamepad",
    88,48,
    0,
    false,
    XGamePadDefault,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

void M_XGamePadChoice(int choice) {
    float slope1 = 0.0125f / 100.0f;
    float slope2 = 100.0f / 50.0f;

    switch(itemOn) {
    case xgp_sensitivity:
        if(choice) {
            if(i_rsticksensitivity.value < 0.0125f) {
                M_SetCvar(&i_rsticksensitivity, i_rsticksensitivity.value + slope1);
            }
            else {
                CON_CvarSetValue(i_rsticksensitivity.name, 0.0125f);
            }
        }
        else {
            if(i_rsticksensitivity.value > 0.001f) {
                M_SetCvar(&i_rsticksensitivity, i_rsticksensitivity.value - slope1);
            }
            else {
                CON_CvarSetValue(i_rsticksensitivity.name, 0.001f);
            }
        }
        break;

    case xgp_threshold:
        if(choice) {
            if(i_rstickthreshold.value < 100) {
                M_SetCvar(&i_rstickthreshold, i_rstickthreshold.value + slope2);
            }
            else {
                CON_CvarSetValue(i_rstickthreshold.name, 100);
            }
        }
        else {
            if(i_rstickthreshold.value > 1) {
                M_SetCvar(&i_rstickthreshold, i_rstickthreshold.value - slope2);
            }
            else {
                CON_CvarSetValue(i_rstickthreshold.name, 1);
            }
        }
        break;
    }
}

void M_DrawXGamePad(void) {
    M_DrawThermo(XGamePadDef.x, XGamePadDef.y + LINEHEIGHT*(xgp_sensitivity+1),
                 100, i_rsticksensitivity.value * 8000.0f);

    M_DrawThermo(XGamePadDef.x, XGamePadDef.y + LINEHEIGHT*(xgp_threshold+1),
                 50, i_rstickthreshold.value * 0.5f);

    Draw_BigText(XGamePadDef.x + 128, XGamePadDef.y + LINEHEIGHT * xgp_look, MENUCOLORRED,
                 msgNames[(int)v_mlook.value]);

    Draw_BigText(XGamePadDef.x + 128, XGamePadDef.y + LINEHEIGHT * xgp_invert, MENUCOLORRED,
                 msgNames[(int)v_mlookinvert.value]);
}

#endif  // XINPUT

//------------------------------------------------------------------------
//
// CONTROLS MENU
//
//------------------------------------------------------------------------

void M_ChangeKeyBinding(int choice);
void M_BuildControlMenu(void);
void M_DrawControls(void);

#define NUM_NONBINDABLE_ITEMS   8
#define NUM_CONTROL_ACTIONS     44
#define NUM_CONTROL_ITEMS        NUM_CONTROL_ACTIONS + NUM_NONBINDABLE_ITEMS

menuaction_t*   PlayerActions;
menu_t          ControlsDef;
menuitem_t      ControlsItem[NUM_CONTROL_ITEMS];

static menuaction_t mPlayerActionsDef[NUM_CONTROL_ITEMS] = {
    {"Movement", NULL},
    {"Forward", "+forward"},
    {"Back", "+back"},
    {"Left", "+left"},
    {"Right", "+right"},
    {"Strafe", "+strafe"},
    {"Strafe Left", "+strafeleft"},
    {"Strafe Right", "+straferight"},
    {"Action", NULL},
    {"Fire", "+fire"},
    {"Use", "+use"},
    {"Run", "+run"},
    {"Jump", "+jump"},
    {"Autorun", "autorun"},
    {"Look Up", "+lookup"},
    {"Look Down", "+lookdown"},
    {"Center View", "+center"},
    {"Weapons", NULL},
    {"Next Weapon", "nextweap"},
    {"Previous Weapon", "prevweap"},
    {"Fist", "weapon 2"},
    {"Pistol", "weapon 3"},
    {"Shotgun", "weapon 4"},
    {"Chaingun", "weapon 6"},
    {"Rocket Launcher", "weapon 7"},
    {"Plasma Rifle", "weapon 8"},
    {"BFG 9000", "weapon 9"},
    {"Chainsaw", "weapon 1"},
    {"Super Shotgun", "weapon 5"},
    {"Laser Artifact", "weapon 10"},
    {"Automap", NULL},
    {"Toggle", "automap"},
    {"Zoom In", "+automap_in"},
    {"Zoom Out", "+automap_out"},
    {"Pan Left", "+automap_left"},
    {"Pan Right", "+automap_right"},
    {"Pan Up", "+automap_up"},
    {"Pan Down", "+automap_down"},
    {"Mouse Pan", "+automap_freepan"},
    {"Follow Mode", "automap_follow"},
    {"Other", NULL},
    {"Detatch Camera", "setcamerastatic"},
    {"Chasecam", "setcamerachase"},
    {NULL, NULL}
};

void M_Controls(int choice) {
    M_SetupNextMenu(&ControlMenuDef);
}

void M_BuildControlMenu(void) {
    menu_t        *menu;
    int            actions;
    int            item;
    int            i;

    PlayerActions = mPlayerActionsDef;

    actions = 0;
    while(PlayerActions[actions].name) {
        actions++;
    }

    menu = &ControlsDef;
    // add extra menu items for non-bindable items (display only)
    menu->numitems = actions + NUM_NONBINDABLE_ITEMS;
    menu->textonly = false;
    menu->numpageitems = 16;
    menu->prevMenu = &ControlMenuDef;
    menu->menuitems = ControlsItem;
    menu->routine = M_DrawControls;
    menu->x = 120;
    menu->y = 80;
    menu->smallfont = true;
    menu->menupageoffset = 0;
    menu->scale = 0.75f;
    sprintf(menu->title, "Bindings");
    menu->lastOn = itemOn;

    for(item = 0; item < actions; item++) {
        dstrcpy(menu->menuitems[item].name, PlayerActions[item].name);
        if(PlayerActions[item].action) {
            for(i = dstrlen(PlayerActions[item].name); i < 15; i++) {
                menu->menuitems[item].name[i] = ' ';
            }

            menu->menuitems[item].name[15] = ':';
            menu->menuitems[item].status = 1;
            menu->menuitems[item].routine = M_ChangeKeyBinding;

            G_GetActionBindings(&menu->menuitems[item].name[16], PlayerActions[item].action);
        }
        else {
            menu->menuitems[item].status = -1;
            menu->menuitems[item].routine = NULL;
        }

        menu->menuitems[item].alphaKey = 0;
    }

#define ADD_NONBINDABLE_ITEM(i, str, s)                 \
    dstrcpy(menu->menuitems[actions + i].name, str);    \
    menu->menuitems[actions + i].status = s;            \
    menu->menuitems[actions + i].routine = NULL

    ADD_NONBINDABLE_ITEM(0, "Non-Bindable Keys", -1);
    ADD_NONBINDABLE_ITEM(1, "Save Game      : F2", 1);
    ADD_NONBINDABLE_ITEM(2, "Load Game      : F3", 1);
    ADD_NONBINDABLE_ITEM(3, "Screenshot     : F5", 1);
    ADD_NONBINDABLE_ITEM(4, "Quicksave      : F6", 1);
    ADD_NONBINDABLE_ITEM(5, "Quickload      : F7", 1);
    ADD_NONBINDABLE_ITEM(6, "Change Gamma   : F11", 1);
    ADD_NONBINDABLE_ITEM(7, "Chat           : t", 1);
}

void M_ChangeKeyBinding(int choice) {
    char action[128];
    sprintf(action, "%s %d", PlayerActions[choice].action, 1);
    dstrcpy(MenuBindBuff, action);
    messageBindCommand=MenuBindBuff;
    sprintf(MenuBindMessage, "%s", PlayerActions[choice].name);
    MenuBindActive = true;
}

void M_DrawControls(void) {
    Draw_BigText(-1, 264, MENUCOLORWHITE , "Press Escape To Return");
    Draw_BigText(-1, 280, MENUCOLORWHITE , "Press Delete To Unbind");
}

//------------------------------------------------------------------------
//
// CONTROLS MENU
//
//------------------------------------------------------------------------

void M_ControlChoice(int choice);
void M_DrawControlMenu(void);

enum {
    controls_keyboard,
    controls_mouse,
#ifdef _USE_XINPUT  // XINPUT
    controls_gamepad,
#endif
    controls_return,
    controls_end
};

menuitem_t ControlsMenu[]= {
    {1,"Bindings",M_ControlChoice, 'k'},
    {1,"Mouse",M_ControlChoice, 'm'},
#ifdef _USE_XINPUT  // XINPUT
    {1,"GamePad",M_ControlChoice, 'g'},
#endif
    {1,"/r Return",M_Return, 0x20}
};

const char* ControlsHints[controls_end]= {
    "configure bindings",
    "configure mouse functionality",
#ifdef _USE_XINPUT  // XINPUT
    "configure gamepad functionality",
#endif
    NULL
};

menu_t ControlMenuDef = {
    controls_end,
    false,
    &OptionsDef,
    ControlsMenu,
    M_DrawControlMenu,
    "Controls",
    120,64,
    0,
    false,
    NULL,
    -1,
    0,
    1,
    ControlsHints,
    NULL
};

void M_ControlChoice(int choice) {
    switch(itemOn) {
    case controls_keyboard:
        M_BuildControlMenu();
        M_SetupNextMenu(&ControlsDef);
        break;
    case controls_mouse:
        M_SetupNextMenu(&MouseDef);
        break;
#ifdef _USE_XINPUT  // XINPUT
    case controls_gamepad:
        M_SetupNextMenu(&XGamePadDef);
        break;
#endif
    }
}

void M_DrawControlMenu(void) {
    if(ControlMenuDef.hints[itemOn] != NULL) {
        GL_SetOrthoScale(0.5f);
        Draw_BigText(-1, 410, MENUCOLORWHITE, ControlMenuDef.hints[itemOn]);
        GL_SetOrthoScale(ControlMenuDef.scale);
    }
}

//------------------------------------------------------------------------
//
// QUICKSAVE CONFIRMATION
//
//------------------------------------------------------------------------

void M_DrawQuickSaveConfirm(void);

enum {
    QS_Ok = 0,
    QS_End
};

menuitem_t QuickSaveConfirm[]= {
    {1,"Ok",M_ReturnToOptions,'o'}
};

menu_t QuickSaveConfirmDef = {
    QS_End,
    false,
    &PauseDef,
    QuickSaveConfirm,
    M_DrawQuickSaveConfirm,
    " ",
    144,112,
    QS_Ok,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

void M_DrawQuickSaveConfirm(void) {
    Draw_BigText(-1, 16, MENUCOLORRED , "You Need To Pick");
    Draw_BigText(-1, 32, MENUCOLORRED , "A Quicksave Slot!");
}

//------------------------------------------------------------------------
//
// LOAD IN NETGAME NOTIFY
//
//------------------------------------------------------------------------

void M_DrawNetLoadNotify(void);

enum {
    NLN_Ok = 0,
    NLN_End
};

menuitem_t NetLoadNotify[]= {
    {1,"Ok",M_ReturnToOptions,'o'}
};

menu_t NetLoadNotifyDef = {
    NLN_End,
    false,
    &PauseDef,
    NetLoadNotify,
    M_DrawNetLoadNotify,
    " ",
    144,112,
    NLN_Ok,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

void M_DrawNetLoadNotify(void) {
    Draw_BigText(-1, 16, MENUCOLORRED , "You Cannot Load While");
    Draw_BigText(-1, 32, MENUCOLORRED , "In A Net Game!");
}

//------------------------------------------------------------------------
//
// SAVEDEAD NOTIFY
//
//------------------------------------------------------------------------

void M_DrawSaveDeadNotify(void);

enum {
    SDN_Ok = 0,
    SDN_End
};

menuitem_t SaveDeadNotify[]= {
    {1,"Ok",M_ReturnToOptions,'o'}
};

menu_t SaveDeadDef = {
    SDN_End,
    false,
    &PauseDef,
    SaveDeadNotify,
    M_DrawSaveDeadNotify,
    " ",
    144,112,
    SDN_Ok,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

void M_DrawSaveDeadNotify(void) {
    Draw_BigText(-1, 16, MENUCOLORRED , "You Cannot Save");
    Draw_BigText(-1, 32, MENUCOLORRED , "While Not In Game");
}

//------------------------------------------------------------------------
//
// SAVE GAME MENU
//
//------------------------------------------------------------------------

void M_SaveSelect(int choice);
void M_ReadSaveStrings(void);
void M_DrawSave(void);

enum {
    load1,
    load2,
    load3,
    load4,
    load5,
    load6,
    load7,
    load8,
    load_end
};

menuitem_t SaveMenu[]= {
    {1,"", M_SaveSelect,'1'},
    {1,"", M_SaveSelect,'2'},
    {1,"", M_SaveSelect,'3'},
    {1,"", M_SaveSelect,'4'},
    {1,"", M_SaveSelect,'5'},
    {1,"", M_SaveSelect,'6'},
    {1,"", M_SaveSelect,'7'},
    {1,"", M_SaveSelect,'8'},
};

menu_t SaveDef = {
    load_end,
    false,
    &PauseDef,
    SaveMenu,
    M_DrawSave,
    "Save Game",
    112,144,
    0,
    false,
    NULL,
    -1,
    0,
    0.5f,
    NULL,
    NULL
};

//
//  M_SaveGame & Cie.
//
void M_DrawSave(void) {
    int i;

    M_DrawSaveGameFrontend(&SaveDef);

    for(i = 0; i < load_end; i++) {
        char *string;

        if(i == saveSlot && inputEnter) {
            string = inputString;
        }
        else {
            string = savegamestrings[i];
        }

        Draw_BigText(SaveDef.x, SaveDef.y + LINEHEIGHT * i, MENUCOLORRED, string);
    }

    if(inputEnter) {
        i = ((int)(160.0f / SaveDef.scale) - Center_Text(inputString)) * 2;
        Draw_BigText(SaveDef.x + i, (SaveDef.y + LINEHEIGHT * saveSlot) - 2, MENUCOLORWHITE, "/r");
    }
}

//
// M_Responder calls this when user is finished
//
void M_DoSave(int slot) {
    G_SaveGame(slot,savegamestrings[slot]);
    M_ClearMenus();

    // PICK QUICKSAVE SLOT YET?
    if(quickSaveSlot == -2) {
        quickSaveSlot = slot;
    }
}

//
// User wants to save. Start string input for M_Responder
//
void M_SaveSelect(int choice) {
    saveSlot = choice;
    dstrcpy(inputString, savegamestrings[choice]);
    M_SetInputString(savegamestrings[choice], (SAVESTRINGSIZE - 1));
}

//
// Selected from DOOM menu
//
void M_SaveGame(int choice) {
    if(!usergame) {
        M_StartControlPanel(true);
        M_SetupNextMenu(&SaveDeadDef);
        return;
    }

    if(gamestate != GS_LEVEL) {
        return;
    }

    M_SetupNextMenu(&SaveDef);
    M_ReadSaveStrings();
}

//------------------------------------------------------------------------
//
// LOAD GAME MENU
//
//------------------------------------------------------------------------

void M_LoadSelect(int choice);
void M_DrawLoad(void);

menuitem_t DoomLoadMenu[]= { //LoadMenu conflicts with Win32 API
    {1,"", M_LoadSelect,'1'},
    {1,"", M_LoadSelect,'2'},
    {1,"", M_LoadSelect,'3'},
    {1,"", M_LoadSelect,'4'},
    {1,"", M_LoadSelect,'5'},
    {1,"", M_LoadSelect,'6'},
    {1,"", M_LoadSelect,'7'},
    {1,"", M_LoadSelect,'8'}
};

menu_t LoadMainDef = {
    load_end,
    false,
    &MainDef,
    DoomLoadMenu,
    M_DrawLoad,
    "Load Game",
    112,144,
    0,
    false,
    NULL,
    -1,
    0,
    0.5f,
    NULL,
    NULL
};

menu_t LoadDef = {
    load_end,
    false,
    &PauseDef,
    DoomLoadMenu,
    M_DrawLoad,
    "Load Game",
    112,144,
    0,
    false,
    NULL,
    -1,
    0,
    0.5f,
    NULL,
    NULL
};

//
// M_LoadGame & Cie.
//
void M_DrawLoad(void) {
    int i;

    M_DrawSaveGameFrontend(&LoadDef);

    for(i = 0; i < load_end; i++)
        Draw_BigText(LoadDef.x, LoadDef.y + LINEHEIGHT * i,
                     MENUCOLORRED, savegamestrings[i]);
}


//
// User wants to load this game
//
void M_LoadSelect(int choice) {
    //char name[256];

    // sprintf(name, SAVEGAMENAME"%d.dsg", choice);
    // G_LoadGame(name);
    char* save_name = P_GetSaveGameName(choice);
    G_LoadGame(save_name);
    free(save_name);
    M_ClearMenus();
}

//
// Selected from DOOM menu
//
void M_LoadGame(int choice) {
    if(netgame) {
        M_StartControlPanel(true);
        M_SetupNextMenu(&NetLoadNotifyDef);
        return;
    }

    if(currentMenu == &MainDef) {
        M_SetupNextMenu(&LoadMainDef);
    }
    else {
        M_SetupNextMenu(&LoadDef);
    }

    M_ReadSaveStrings();
}

//
// M_ReadSaveStrings
//
// Read the strings from the savegame files
//
void M_ReadSaveStrings(void) {
    int     handle;
    int     i;
    // char    name[256];

    for(i = 0; i < load_end; i++) {
        // sprintf(name, SAVEGAMENAME"%d.dsg", i);

        // handle = open(name, O_RDONLY | 0, 0666);
        char* save_name = P_GetSaveGameName(i);
        handle = open(save_name, O_RDONLY | 0, 0666);
        free(save_name); // Allocated by I_GetUserFile
        if(handle == -1) {
            dstrcpy(&savegamestrings[i][0],EMPTYSTRING);
            DoomLoadMenu[i].status = 0;
            continue;
        }
        read(handle, &savegamestrings[i], MENUSTRINGSIZE);
        close(handle);
        DoomLoadMenu[i].status = 1;
    }
}

//------------------------------------------------------------------------
//
// QUICKSAVE PROMPT
//
//------------------------------------------------------------------------

void M_QuickSaveResponse(int ch);

enum {
    QSP_Yes = 0,
    QSP_No,
    QSP_End
};

menuitem_t QuickSavePrompt[]= {
    {1,"Yes",M_QuickSaveResponse,'y'},
    {1,"No",M_ReturnToOptions,'n'}
};

menu_t QuickSavePromptDef = {
    QSP_End,
    false,
    &PauseDef,
    QuickSavePrompt,
    NULL,
    "Overwrite QuickSave?",
    144,112,
    QSP_Yes,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

void M_QuickSaveResponse(int ch) {
    M_DoSave(quickSaveSlot);
}

//------------------------------------------------------------------------
//
// QUICKLOAD PROMPT
//
//------------------------------------------------------------------------

void M_QuickLoadResponse(int ch);

enum {
    QLP_Yes = 0,
    QLP_No,
    QLP_End
};

menuitem_t QuickLoadPrompt[]= {
    {1,"Yes",M_QuickLoadResponse,'y'},
    {1,"No",M_ReturnToOptions,'n'}
};

menu_t QuickLoadPromptDef = {
    QLP_End,
    false,
    &PauseDef,
    QuickLoadPrompt,
    NULL,
    "Load QuickSave?",
    144,112,
    QLP_Yes,
    false,
    NULL,
    -1,
    0,
    1.0f,
    NULL,
    NULL
};

void M_QuickLoadResponse(int ch) {
    M_LoadSelect(quickSaveSlot);
}

//------------------------------------------------------------------------
//
// COMMON MENU FUNCTIONS
//
//------------------------------------------------------------------------

//
// M_SetCvar
//

static int prevtic = 0; // hack - check for overlapping sounds
template <class T, class U>
static void M_SetCvar(TypedProperty<T> &property, const U &value) {
    if(property == value) {
        return;
    }

    if(prevtic != gametic) {
        S_StartSound(NULL,
                     currentMenu->menuitems[itemOn].status == 3 ? sfx_secmove : sfx_switch2);

        prevtic = gametic;
    }

    property = value;
}

//
// M_SetOptionValue
//

static void M_SetOptionValue(int choice, BoolProperty &property)
{
    M_SetCvar(property, !property);
}

template <class T, class U>
static void M_SetOptionValue(int choice, U min, U max, U inc, TypedProperty<T> &cvar) {
    if(choice) {
        if(cvar < max) {
            M_SetCvar(cvar, cvar + inc);
        }
        else if(choice == 2) {
            M_SetCvar(cvar, min);
        }
        else {
            cvar = max;
        }
    }
    else {
        if(cvar > min) {
            M_SetCvar(cvar, cvar - inc);
        }
        else if(choice == 2) {
            M_SetCvar(cvar, max);
        }
        else {
            cvar = min;
        }
    }
}

//
// M_DoDefaults
//

static void M_DoDefaults(int choice) {
    // FIXME: Allow defaults
#if 0
    int i = 0;

    for(i = 0; currentMenu->defaultitems[i].mitem != NULL; i++) {
        currentMenu->defaultitems[i].mitem->name, currentMenu->defaultitems[i].mdefault);
    }

    if(currentMenu == &DisplayDef) {
        R_RefreshBrightness();
    }

    if(currentMenu == &VideoDef) {
        CON_CvarSetValue(v_width.name, 640);
        CON_CvarSetValue(v_height.name, 480);

        GL_DumpTextures();
        GL_SetTextureFilter();
    }
#endif

    S_StartSound(NULL, sfx_switch2);
}

//
// M_ReturnToOptions
//

void M_ReturnToOptions(int choice) {
    M_SetupNextMenu(&PauseDef);
}

//
// M_Return
//

static void M_Return(int choice) {
    if (currentMenu == &VideoDef)
        M_VideoSetMode();

    currentMenu->lastOn = itemOn;
    if(currentMenu->prevMenu) {
        menufadefunc = M_MenuFadeOut;
        alphaprevmenu = true;
        S_StartSound(NULL, sfx_pistol);
    }
}

//
// M_ReturnInstant
//

static void M_ReturnInstant(void) {
    if (currentMenu == &VideoDef)
        M_VideoSetMode();

    if(currentMenu->prevMenu) {
        currentMenu = currentMenu->prevMenu;
        itemOn = currentMenu->lastOn;

        S_StartSound(NULL, sfx_switch2);
    }
    else {
        M_ClearMenus();
    }
}

//
// M_SetInputString
//

static void M_SetInputString(char* string, int len) {
    inputEnter = true;
    dstrcpy(oldInputString, string);

    // hack
    if(!dstrcmp(string, EMPTYSTRING)) {
        inputString[0] = 0;
    }

    inputCharIndex = dstrlen(inputString);
    inputMax = len;
}

//
// M_DrawSmbString
//

static void M_DrawSmbString(const char* text, menu_t* menu, int item) {
    int x;
    int y;

    x = menu->x + 128;
    y = menu->y + (ST_FONTWHSIZE + 1) * item;
    Draw_Text(x, y, MENUCOLORWHITE, 1.0f, false, text);
}

//
// M_StringWidth
// Find string width from hu_font chars
//

static int M_StringWidth(const char* string) {
    int i;
    int w = 0;
    int c;

    for(i = 0; i < dstrlen(string); i++) {
        c = toupper(string[i]) - ST_FONTSTART;
        if(c < 0 || c >= ST_FONTSIZE) {
            w += 4;
        }
        else {
            w += ST_FONTWHSIZE;
        }
    }

    return w;
}

//
// M_BigStringWidth
// Find string width from bigfont chars
//

static int M_BigStringWidth(const char* string) {
    int width = 0;
    char t = 0;
    int id = 0;
    int len = 0;
    int i = 0;

    len = dstrlen(string);

    for(i = 0; i < len; i++) {
        t = string[i];

        switch(t) {
        case 0x20:
            width += 6;
            break;
        case '-':
            width += symboldata[SM_MISCFONT].w;
            break;
        case '%':
            width += symboldata[SM_MISCFONT + 1].w;
            break;
        case '!':
            width += symboldata[SM_MISCFONT + 2].w;
            break;
        case '.':
            width += symboldata[SM_MISCFONT + 3].w;
            break;
        case '?':
            width += symboldata[SM_MISCFONT + 4].w;
            break;
        case ':':
            width += symboldata[SM_MISCFONT + 5].w;
            break;
        default:
            if(t >= 'A' && t <= 'Z') {
                id = t - 'A';
                width += symboldata[SM_FONT1 + id].w;
            }
            if(t >= 'a' && t <= 'z') {
                id = t - 'a';
                width += symboldata[SM_FONT2 + id].w;
            }
            if(t >= '0' && t <= '9') {
                id = t - '0';
                width += symboldata[SM_NUMBERS + id].w;
            }
            break;
        }
    }

    return width;
}

//
// M_Scroll
//
// Allow scrolling through multi-page menus via mouse wheel
//

static void M_Scroll(menu_t* menu, dboolean up) {
    if(menu->numpageitems != -1) {
        if(!up) {
            menu->menupageoffset++;
            if(menu->numpageitems + menu->menupageoffset >=
                    menu->numitems) {
                menu->menupageoffset =
                    menu->numitems - menu->numpageitems;
            }
            else if(++itemOn >= menu->numitems) {
                itemOn = menu->numitems - 1;
            }
        }
        else {
            menu->menupageoffset--;
            if(menu->menupageoffset < 0) {
                menu->menupageoffset = 0;
                if(itemOn < 0) {
                    itemOn = 0;
                }
            }
            else if(--itemOn < 0) {
                itemOn = 0;
            }
        }
    }
}

//
// M_CheckDragThermoBar
//
// Allow user to click and drag thermo bar
//
// To be fair, this is a horrid mess and a awful hack...
// Really need a better and more efficient menu system
//

static void M_CheckDragThermoBar(event_t* ev, menu_t* menu) {
    menuthermobar_t *bar;
    float startx;
    float scrny;
    int x;
    int y;
    int i;
    float mx;
    float my;
    float width;
    float scalex;
    float scaley;
    float lineheight;

    // must be a mouse held event and menu must have thermobar settings
    if(ev->type != ev_mouse || menu->thermobars == NULL) {
        return;
    }

    // mouse buttons must be held and moving
    if(!(ev->data1 & 1) || !(ev->data2 | ev->data3)) {
        return;
    }

    bar = menu->thermobars;
    x = menu->x;
    y = menu->y;
    mx = (float)mouse_x;
    my = (float)mouse_y;
    scalex = ((float)video_width /
              ((float)SCREENHEIGHT * video_ratio)) * menu->scale;
    scaley = ((float)video_height /
              (float)SCREENHEIGHT) * menu->scale;
    startx = (float)x * scalex;
    width = startx + (100.0f * scalex);

    // check if cursor is within range
    for(i = 0; bar[i].item != -1; i++) {
        lineheight = (float)(LINEHEIGHT * bar[i].item);
        scrny = ((float)y + lineheight + 10) * scaley;

        if(my < scrny) {
            scrny = ((float)y + lineheight + 2) * scaley;
            if(my >= scrny) {
                // dragged all the way to the left?
                if(mx < startx) {
//                    CON_CvarSetValue(bar[i].mitem->name, 0);
                    return;
                }

                // dragged all the way to the right?
                if(mx > width) {
//                    CON_CvarSetValue(bar[i].mitem->name, bar[i].width);
                    return;
                }

                // convert mouse x coordinate into thermo bar position
                // set cvar as well
//                value = (mx / scalex) - x;
//                CON_CvarSetValue(bar[i].mitem->name,
//                                 value * (bar[i].width / 100.0f));

                return;
            }
        }
    }
}

//
// M_CursorHighlightItem
//
// Highlight effects when positioning mouse cursor
// over menu item
//
// To be fair, this is a horrid mess and a awful hack...
// Really need a better and more efficient menu system
//

static dboolean M_CursorHighlightItem(menu_t* menu) {
    float scrnx;
    float scrny;
    float mx;
    float my;
    int width;
    int height;
    float scalex;
    float scaley;
    int item;
    int max;
    int start;
    int x;
    int y;
    int lineheight;

    start = menu->menupageoffset;
    max = (menu->numpageitems == -1) ? menu->numitems : menu->numpageitems;
    x = menu->x;
    y = menu->y;
    mx = (float)mouse_x;
    my = (float)mouse_y;
    scalex = ((float)video_width /
              ((float)SCREENHEIGHT * video_ratio)) * menu->scale;
    scaley = ((float)video_height /
              (float)SCREENHEIGHT) * menu->scale;

    if(menu->textonly) {
        lineheight = TEXTLINEHEIGHT;
    }
    else {
        lineheight = LINEHEIGHT;
    }

    if(menu->smallfont) {
        lineheight /= 2;
    }

    itemSelected = -1;

    for(item = start; item < max + start; item++) {
        // skip hidden items
        if(menu->menuitems[item].status == -3) {
            continue;
        }

        if(menu == &PasswordDef) {
            if(item > 0) {
                if(!(item & 7)) {
                    y += lineheight;
                    x = menu->x;
                }
                else {
                    x += TEXTLINEHEIGHT;
                }
            }
        }

        // highlight non-static items
        if(menu->menuitems[item].status != -1) {
            // guess the bounding box size based on string length and font type
            width = (menu->smallfont ?
                     M_StringWidth(menu->menuitems[item].name) :
                     M_BigStringWidth(menu->menuitems[item].name));
            height = (menu->smallfont ? 8 : LINEHEIGHT);
            scrnx = (float)(x + width) * scalex;
            scrny = (float)(y + height) * scaley;

            // do extra checks for columns if we're in the password menu
            // otherwise we'll just care about rows

            if(mx < scrnx || menu != &PasswordDef) {
                scrnx = (float)x * scalex;
                if(mx >= scrnx || menu != &PasswordDef) {
                    if(my < scrny) {
                        scrny = (float)y * scaley;
                        if(my >= scrny) {
                            itemSelected = item;
                            return true;
                        }
                    }
                }
            }
        }

        if(menu != &PasswordDef) {
            y += lineheight;
        }
    }

    return false;
}

//
// M_QuickSave
//

//
// M_QuickSave
//

void M_QuickSave(void) {
    if(!usergame) {
        S_StartSound(NULL,sfx_oof);
        return;
    }

    if(gamestate != GS_LEVEL) {
        return;
    }

    if(quickSaveSlot < 0) {
        M_StartControlPanel(true);
        M_ReadSaveStrings();
        M_SetupNextMenu(&SaveDef);
        quickSaveSlot = -2;     // means to pick a slot now
        return;
    }

    M_StartControlPanel(true);
    M_SetupNextMenu(&QuickSavePromptDef);
}


void M_QuickLoad(void) {
    if(netgame) {
        M_StartControlPanel(true);
        M_SetupNextMenu(&NetLoadNotifyDef);
        return;
    }

    if(quickSaveSlot < 0) {
        M_StartControlPanel(true);
        M_SetupNextMenu(&QuickSaveConfirmDef);
        return;
    }

    M_StartControlPanel(true);
    M_SetupNextMenu(&QuickLoadPromptDef);
}


//
// Menu Functions
//

static void M_DrawThermo(int x, int y, int thermWidth, float thermDot) {
    float slope = 100.0f / (float)thermWidth;

    Draw_BigText(x, y, MENUCOLORWHITE, "/t");
    Draw_BigText(x + (int)(thermDot * slope) * (symboldata[SM_THERMO].w / 100), y, MENUCOLORWHITE, "/s");
}

//
// M_SetThumbnail
//

static dtexture thumbnail = 0;
static char thumbnail_date[32];
static int thumbnail_skill = -1;
static int thumbnail_map = -1;

static dboolean M_SetThumbnail(int which) {
    byte* data;

    data = (byte*) Z_Malloc(SAVEGAMETBSIZE, PU_STATIC, 0);

    //
    // poke into savegame file and fetch
    // thumbnail, date and stats
    //
    char* save_name = P_GetSaveGameName(which);
    if(!P_QuickReadSaveHeader(save_name, thumbnail_date, (int*)data,
                              &thumbnail_skill, &thumbnail_map)) {
        free(save_name);
        Z_Free(data);
        return 0;
    }
    free(save_name);

    //
    // make a new thumbnail texture
    //
    if(thumbnail == 0) {
        dglGenTextures(1, &thumbnail);
        dglBindTexture(GL_TEXTURE_2D, thumbnail);

        GL_SetTextureFilter();

        dglTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGB8,
            128,
            128,
            0,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            data
        );
    }
    else {
        dglBindTexture(GL_TEXTURE_2D, thumbnail);

        GL_SetTextureFilter();

        dglTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            0,
            128,
            128,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            data
        );
    }

    Z_Free(data);

    return 1;
}

//
// M_DrawSaveGameFrontend
//

static void M_DrawSaveGameFrontend(menu_t* def) {
    GL_SetState(GLSTATE_BLEND, 1);
    GL_SetOrtho(0);

    dglDisable(GL_TEXTURE_2D);

    //
    // draw back panels
    //
    dglColor4ub(32, 32, 32, menualphacolor);
    //
    // save game panel
    //
    dglRecti(
        def->x - 48,
        def->y - 12,
        def->x + 256,
        def->y + 156
    );
    //
    // thumbnail panel
    //
    dglRecti(
        def->x + 272,
        def->y - 12,
        def->x + 464,
        def->y + 116
    );
    //
    // stats panel
    //
    dglRecti(
        def->x + 272,
        def->y + 124,
        def->x + 464,
        def->y + 176
    );

    //
    // draw outline for panels
    //
    dglColor4ub(192, 192, 192, menualphacolor);
    dglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //
    // save game panel
    //
    dglRecti(
        def->x - 48,
        def->y - 12,
        def->x + 256,
        def->y + 156
    );
    //
    // thumbnail panel
    //
    dglRecti(
        def->x + 272,
        def->y - 12,
        def->x + 464,
        def->y + 116
    );
    //
    // stats panel
    //
    dglRecti(
        def->x + 272,
        def->y + 124,
        def->x + 464,
        def->y + 176
    );
    dglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    dglEnable(GL_TEXTURE_2D);

    //
    // 20120404 villsa - reset active textures just to make sure
    // we don't end up seeing that thumbnail texture on a wall or something
    //
    GL_ResetTextures();

    //
    // draw thumbnail texture and stats
    //
    if(M_SetThumbnail(itemOn)) {
        char string[128];

        dglBindTexture(GL_TEXTURE_2D, thumbnail);

        dglBegin(GL_POLYGON);
        dglColor4ub(0xff, 0xff, 0xff, menualphacolor);
        dglTexCoord2f(0, 0);
        dglVertex2i(def->x + 288, def->y + -8);
        dglTexCoord2f(1, 0);
        dglVertex2i(def->x + 448, def->y + -8);
        dglTexCoord2f(1, 1);
        dglVertex2i(def->x + 448, def->y + 112);
        dglTexCoord2f(0, 1);
        dglVertex2i(def->x + 288, def->y + 112);
        dglEnd();

        curgfx = -1;

        GL_SetOrthoScale(0.35f);

        Draw_BigText(def->x + 444, def->y + 244, MENUCOLORWHITE, thumbnail_date);

        sprintf(string, "Skill: %s", NewGameMenu[thumbnail_skill].name);
        Draw_BigText(def->x + 444, def->y + 268, MENUCOLORWHITE, string);

        sprintf(string, "Map: %s", P_GetMapInfo(thumbnail_map)->mapname);
        Draw_BigText(def->x + 444, def->y + 292, MENUCOLORWHITE, string);

        GL_SetOrthoScale(def->scale);
    }

    GL_SetState(GLSTATE_BLEND, 0);
}



//------------------------------------------------------------------------
//
// CONTROL PANEL
//
//------------------------------------------------------------------------

#ifdef _USE_XINPUT  // XINPUT

const symboldata_t xinputbutons[12] = {
    { 0, 0, 15, 16 },   // B
    { 15, 0, 15, 16 },  // A
    { 30, 0, 15, 16 },  // Y
    { 45, 0, 15, 16 },  // X
    { 60, 0, 19, 16 },  // LB
    { 79, 0, 19, 16 },  // RB
    { 98, 0, 15, 16 },  // LEFT
    { 113, 0, 15, 16 }, // RIGHT
    { 128, 0, 15, 16 }, // UP
    { 143, 0, 15, 16 }, // DOWN
    { 158, 0, 12, 16 }, // START
    { 170, 0, 12, 16 }  // SELECT
};

//
// M_DrawXInputButton
//

void M_DrawXInputButton(int x, int y, int button) {
    int index = 0;
    float vx1 = 0.0f;
    float vy1 = 0.0f;
    float tx1 = 0.0f;
    float tx2 = 0.0f;
    float ty1 = 0.0f;
    float ty2 = 0.0f;
    float width;
    float height;
    int pic;
    vtx_t vtx[4];
    const rcolor color = MENUCOLORWHITE;

    switch(button) {
    case XINPUT_GAMEPAD_B:
        index = 0;
        break;
    case XINPUT_GAMEPAD_A:
        index = 1;
        break;
    case XINPUT_GAMEPAD_Y:
        index = 2;
        break;
    case XINPUT_GAMEPAD_X:
        index = 3;
        break;
    case XINPUT_GAMEPAD_LEFT_SHOULDER:
        index = 4;
        break;
    case XINPUT_GAMEPAD_RIGHT_SHOULDER:
        index = 5;
        break;
    case XINPUT_GAMEPAD_DPAD_LEFT:
        index = 6;
        break;
    case XINPUT_GAMEPAD_DPAD_RIGHT:
        index = 7;
        break;
    case XINPUT_GAMEPAD_DPAD_UP:
        index = 8;
        break;
    case XINPUT_GAMEPAD_DPAD_DOWN:
        index = 9;
        break;
    case XINPUT_GAMEPAD_START:
        index = 10;
        break;
    case XINPUT_GAMEPAD_BACK:
        index = 11;
        break;
    case XINPUT_GAMEPAD_LEFT_TRIGGER:
        index = 4;
        break;
    case XINPUT_GAMEPAD_RIGHT_TRIGGER:
        index = 5;
        break;
    //
    // [kex] TODO: finish adding remaining buttons?
    //
    default:
        return;
    }

    pic = GL_BindGfxTexture("BUTTONS", true);

    width = (float)gfxwidth[pic];
    height = (float)gfxheight[pic];

    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, DGL_CLAMP);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, DGL_CLAMP);

    dglEnable(GL_BLEND);
    dglSetVertex(vtx);

    GL_SetOrtho(0);

    vx1 = (float)x;
    vy1 = (float)y;

    tx1 = ((float)xinputbutons[index].x / width) + 0.001f;
    tx2 = (tx1 + (float)xinputbutons[index].w / width) - 0.001f;
    ty1 = ((float)xinputbutons[index].y / height) + 0.005f;
    ty2 = (ty1 + (((float)xinputbutons[index].h / height))) - 0.008f;

    GL_Set2DQuad(
        vtx,
        vx1,
        vy1,
        xinputbutons[index].w,
        xinputbutons[index].h,
        tx1,
        tx2,
        ty1,
        ty2,
        color
    );

    dglTriangle(0, 1, 2);
    dglTriangle(3, 2, 1);
    dglDrawGeometry(4, vtx);

    GL_ResetViewport();
    dglDisable(GL_BLEND);
}

#endif

//
// M_Responder
//

static dboolean shiftdown = false;

dboolean M_Responder(event_t* ev) {
    int ch;
    int i;

    ch = -1;

    if(menufadefunc || !allowmenu || demoplayback) {
        return false;
    }

    if(ev->type == ev_mousedown) {
        if(ev->data1 & 1) {
            ch = KEY_ENTER;
        }

        if(ev->data1 & 4) {
            ch = KEY_BACKSPACE;
        }
    }
    else if(ev->type == ev_keydown) {
        ch = ev->data1;

        if(ch == KEY_SHIFT) {
            shiftdown = true;
        }
    }
    else if(ev->type == ev_keyup) {
        thermowait = 0;
        if(ev->data1 == KEY_SHIFT) {
            ch = ev->data1;
            shiftdown = false;
        }
    }
    else if(ev->type == ev_mouse && (ev->data2 | ev->data3)) {
        // handle mouse-over selection
        if(m_menumouse) {
            M_CheckDragThermoBar(ev, currentMenu);
            if(M_CursorHighlightItem(currentMenu)) {
                itemOn = itemSelected;
            }
        }
    }

    if(ch == -1) {
        return false;
    }

#ifdef HAVE_XINPUT
    switch(ch) {
    case BUTTON_DPAD_UP:
        ch = KEY_UPARROW;
        break;
    case BUTTON_DPAD_DOWN:
        ch = KEY_DOWNARROW;
        break;
    case BUTTON_DPAD_LEFT:
        ch = KEY_LEFTARROW;
        break;
    case BUTTON_DPAD_RIGHT:
        ch = KEY_RIGHTARROW;
        break;
    case BUTTON_START:
        ch = KEY_ESCAPE;
        break;
    case BUTTON_A:
        ch = KEY_ENTER;
        break;
    case BUTTON_B:
        ch = KEY_DEL;
        break;
    }
#endif //HAVE_XINPUT

    if(MenuBindActive == true) { //key Bindings
        if(ch == KEY_ESCAPE) {
            MenuBindActive = false;
            M_BuildControlMenu();
        }
        else if(G_BindActionByEvent(ev, messageBindCommand)) {
            MenuBindActive = false;
            M_BuildControlMenu();
        }
        return true;
    }

    // save game / player name string input
    if(inputEnter) {
        switch(ch) {
        case KEY_BACKSPACE:
            if(inputCharIndex > 0) {
                inputCharIndex--;
                inputString[inputCharIndex] = 0;
            }
            break;

        case KEY_ESCAPE:
            inputEnter = false;
            dstrcpy(inputString, oldInputString);
            break;

        case KEY_ENTER:
            inputEnter = false;
            if(currentMenu == &NetworkDef) {
                m_playername = inputString;
                if(netgame) {
                    NET_SV_UpdateCvars(m_playername);
                }
            }
            else {
                dstrcpy(savegamestrings[saveSlot], inputString);
                if(savegamestrings[saveSlot][0]) {
                    M_DoSave(saveSlot);
                }
            }
            break;

        default:

            if(inputCharIndex >= inputMax) {
                return true;
            }

            if(shiftdown) {
                ch = toupper(ch);
            }

            if(ch != 32) {
                if(ch - ST_FONTSTART < 0 || ch - ST_FONTSTART >= ('z' - ST_FONTSTART + 1)) {
                    break;
                }
            }

            if(ch >= 32 && ch <= 127) {
                if(inputCharIndex < (MENUSTRINGSIZE - 1) &&
                        M_StringWidth(inputString) < (MENUSTRINGSIZE - 2) * 8) {
                    inputString[inputCharIndex++] = ch;
                    inputString[inputCharIndex] = 0;
                }
            }
            break;
        }
        return true;
    }


    // F-Keys
    if(!menuactive) {
        switch(ch) {
        case KEY_F2:            // Save
            M_StartControlPanel(true);
            M_SaveGame(0);
            return true;

        case KEY_F3:            // Load
            M_StartControlPanel(true);
            M_LoadGame(0);
            return true;

        case KEY_F5:
            G_ScreenShot();
            return true;

        case KEY_F6:            // Quicksave
            M_QuickSave();
            return true;

        case KEY_F7:            // Quickload
            M_QuickLoad();
            return true;

        case KEY_F11:           // gamma toggle
            M_ChangeGammaLevel(2);
            return true;
        }
    }


    // Pop-up menu?
    if(!menuactive) {
        if(ch == KEY_ESCAPE && !st_chatOn) {
            M_StartControlPanel(false);
            return true;
        }
        return false;
    }


    // Keys usable within menu
    switch(ch) {
    case KEY_DOWNARROW:
        S_StartSound(NULL,sfx_switch1);
        if(currentMenu == &PasswordDef) {
            itemOn = ((itemOn + 8) & 31);
            return true;
        }
        else {
            do {
                if(itemOn+1 > currentMenu->numitems-1) {
                    itemOn = 0;
                }
                else {
                    itemOn++;
                }
            }
            while(currentMenu->menuitems[itemOn].status==-1 ||
                    currentMenu->menuitems[itemOn].status==-3);
            return true;
        }

    case KEY_UPARROW:
        S_StartSound(NULL,sfx_switch1);
        if(currentMenu == &PasswordDef) {
            itemOn = ((itemOn - 8) & 31);
            return true;
        }
        else {
            do {
                if(!itemOn) {
                    itemOn = currentMenu->numitems-1;
                }
                else {
                    itemOn--;
                }
            }
            while(currentMenu->menuitems[itemOn].status==-1 ||
                    currentMenu->menuitems[itemOn].status==-3);
            return true;
        }

    case KEY_LEFTARROW:
        if(currentMenu == &PasswordDef) {
            S_StartSound(NULL,sfx_switch1);
            do {
                if(!itemOn) {
                    itemOn = currentMenu->numitems-1;
                }
                else {
                    itemOn--;
                }
            }
            while(currentMenu->menuitems[itemOn].status==-1);
            return true;
        }
        else {
            if(currentMenu->menuitems[itemOn].routine &&
                    currentMenu->menuitems[itemOn].status >= 2) {
                currentMenu->menuitems[itemOn].routine(0);

                if(currentMenu->menuitems[itemOn].status == 3) {
                    thermowait = 1;
                }
            }
            return true;
        }

    case KEY_RIGHTARROW:
        if(currentMenu == &PasswordDef) {
            S_StartSound(NULL,sfx_switch1);
            do {
                if(itemOn+1 > currentMenu->numitems-1) {
                    itemOn = 0;
                }
                else {
                    itemOn++;
                }
            }
            while(currentMenu->menuitems[itemOn].status==-1);
            return true;
        }
        else {
            if(currentMenu->menuitems[itemOn].routine &&
                    currentMenu->menuitems[itemOn].status >= 2) {
                currentMenu->menuitems[itemOn].routine(1);

                if(currentMenu->menuitems[itemOn].status == 3) {
                    thermowait = -1;
                }
            }
            return true;
        }

    case KEY_ENTER:
        if(itemOn != itemSelected &&
                ev->type == ev_mousedown) {
            return false;
        }

        if(currentMenu == &PasswordDef) {
            M_PasswordSelect();
            return true;
        }
        else {
            if(currentMenu->menuitems[itemOn].routine &&
                    currentMenu->menuitems[itemOn].status) {
                if(currentMenu->menuitems[itemOn].routine == M_Return) {
                    M_Return(0);
                    return true;
                }

                currentMenu->lastOn = itemOn;
                if(currentMenu == &featuresDef) {
                    if(currentMenu->menuitems[itemOn].routine == M_DoFeature &&
                            itemOn == features_levels) {
                        gameaction = ga_warplevel;
                        gamemap = nextmap = levelwarp + 1;
                        M_ClearMenus();
                        dmemset(passwordData, 0xff, 16);
                        return true;
                    }
                }
                else if(currentMenu->menuitems[itemOn].status >= 2 ||
                        currentMenu->menuitems[itemOn].status == -2) {
                    currentMenu->menuitems[itemOn].routine(2);
                }
                else {
                    if(currentMenu == &ControlsDef) {
                        // don't do the fade effect and jump straight to the next screen
                        M_ChangeKeyBinding(itemOn);
                    }
                    else {
                        currentMenu->menuitems[itemOn].routine(itemOn);
                    }

                    S_StartSound(NULL, sfx_pistol);
                }
            }
            return true;
        }

    case KEY_ESCAPE:
        //villsa
        if(gamestate == GS_SKIPPABLE || demoplayback) {
            return false;
        }

        M_ReturnInstant();
        return true;

    case KEY_DEL:
        if(currentMenu == &PasswordDef) {
            M_PasswordDeSelect();
        }
        else if(currentMenu == &ControlsDef) {
            if(currentMenu->menuitems[itemOn].routine) {
                G_UnbindAction(PlayerActions[itemOn].action);
                M_BuildControlMenu();
            }
        }
        return true;

    case KEY_BACKSPACE:
        M_Return(0);
        return true;

    case KEY_MWHEELUP:
        M_Scroll(currentMenu, true);
        return true;

    case KEY_MWHEELDOWN:
        M_Scroll(currentMenu, false);
        return true;

    case KEY_F5:
        // villsa 12292013 - allow screenshots from menu as well
        G_ScreenShot();
        return true;

    default:
        for(i = itemOn+1; i < currentMenu->numitems; i++) {
            if(currentMenu->menuitems[i].status != -1
                    && currentMenu->menuitems[i].alphaKey == ch) {
                itemOn = i;
                S_StartSound(NULL, sfx_switch1);
                return true;
            }
        }
        for(i = 0; i <= itemOn; i++) {
            if(currentMenu->menuitems[i].status != -1
                    && currentMenu->menuitems[i].alphaKey == ch) {
                itemOn = i;
                S_StartSound(NULL, sfx_switch1);
                return true;
            }
        }
        break;
    }

    return false;
}

//
// M_StartControlPanel
//

void M_StartControlPanel(dboolean forcenext) {
    if(!allowmenu) {
        return;
    }

    if(demoplayback) {
        return;
    }

    // intro might call this repeatedly
    if(menuactive) {
        return;
    }

    menuactive = true;
    menufadefunc = NULL;
    nextmenu = NULL;
    newmenu = forcenext;
    currentMenu = !usergame ? &MainDef : &PauseDef;
    itemOn = currentMenu->lastOn;

    S_PauseSound();
}

//
// M_StartMainMenu
//

void M_StartMainMenu(void) {
    currentMenu = &MainDef;
    itemOn = 0;
    allowmenu = true;
    menuactive = true;
    mainmenuactive = true;
    M_StartControlPanel(false);
}

//
// M_DrawMenuSkull
//
// Draws skull icon from the symbols lump
// Pretty straightforward stuff..
//

static void M_DrawMenuSkull(int x, int y) {
    int index = 0;
    float vx1 = 0.0f;
    float vy1 = 0.0f;
    float tx1 = 0.0f;
    float tx2 = 0.0f;
    float ty1 = 0.0f;
    float ty2 = 0.0f;
    float smbwidth;
    float smbheight;
    int pic;
    vtx_t vtx[4];
    const rcolor color = MENUCOLORWHITE;

    pic = GL_BindGfxTexture("SYMBOLS", true);

    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, DGL_CLAMP);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, DGL_CLAMP);

    dglEnable(GL_BLEND);
    dglSetVertex(vtx);

    GL_SetOrtho(0);

    index = (whichSkull & 7) + SM_SKULLS;

    vx1 = (float)x;
    vy1 = (float)y;

    smbwidth = (float)gfxwidth[pic];
    smbheight = (float)gfxheight[pic];

    tx1 = ((float)symboldata[index].x / smbwidth) + 0.001f;
    tx2 = (tx1 + (float)symboldata[index].w / smbwidth) - 0.001f;
    ty1 = ((float)symboldata[index].y / smbheight) + 0.005f;
    ty2 = (ty1 + (((float)symboldata[index].h / smbheight))) - 0.008f;

    GL_Set2DQuad(
        vtx,
        vx1,
        vy1,
        symboldata[index].w,
        symboldata[index].h,
        tx1,
        tx2,
        ty1,
        ty2,
        color
    );

    dglTriangle(0, 1, 2);
    dglTriangle(3, 2, 1);
    dglDrawGeometry(4, vtx);

    GL_ResetViewport();
    dglDisable(GL_BLEND);
}

//
// M_DrawCursor
//

static void M_DrawCursor(int x, int y) {
    if(m_menumouse) {
        int gfxIdx;
        float factor;
        float scale;

        scale = ((m_cursorscale + 25.0f) / 100.0f);
        gfxIdx = GL_BindGfxTexture("CURSOR", true);
        factor = (((float)SCREENHEIGHT * video_ratio) / (float)video_width) / scale;

        dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, DGL_CLAMP);
        dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, DGL_CLAMP);

        GL_SetOrthoScale(scale);
        GL_SetState(GLSTATE_BLEND, 1);
        GL_SetupAndDraw2DQuad((float)x * factor, (float)y * factor,
                              gfxwidth[gfxIdx], gfxheight[gfxIdx], 0, 1.0f, 0, 1.0f, WHITE, 0);

        GL_SetState(GLSTATE_BLEND, 0);
        GL_SetOrthoScale(1.0f);
    }
}

//
// M_Drawer
//
// Called after the view has been rendered,
// but before it has been blitted.
//

void M_Drawer(void) {
    short x;
    short y;
    short i;
    short max;
    int start;
    int height;

    if(currentMenu != &MainDef) {
        ST_FlashingScreen(0, 0, 0, 96);
    }

    if(MenuBindActive) {
        Draw_BigText(-1, 64, MENUCOLORWHITE, "Press New Key For");
        Draw_BigText(-1, 80, MENUCOLORRED, MenuBindMessage);
        return;
    }

    if(!menuactive) {
        return;
    }

    Draw_BigText(-1, 16, MENUCOLORRED, currentMenu->title);

    if(currentMenu->scale != 1) {
        GL_SetOrthoScale(currentMenu->scale);
    }

    if(currentMenu->routine) {
        currentMenu->routine();    // call Draw routine
    }

    // DRAW MENU
    x = currentMenu->x;
    y = currentMenu->y;

    start = currentMenu->menupageoffset;
    max = (currentMenu->numpageitems == -1) ? currentMenu->numitems : currentMenu->numpageitems;

    if(currentMenu->textonly) {
        height = TEXTLINEHEIGHT;
    }
    else {
        height = LINEHEIGHT;
    }

    if(currentMenu->smallfont) {
        height /= 2;
    }

    //
    // begin drawing all menu items
    //
    for(i = start; i < max+start; i++) {
        //
        // skip hidden items
        //
        if(currentMenu->menuitems[i].status == -3) {
            continue;
        }

        if(currentMenu == &PasswordDef) {
            if(i > 0) {
                if(!(i & 7)) {
                    y += height;
                    x = currentMenu->x;
                }
                else {
                    x += TEXTLINEHEIGHT;
                }
            }
        }

        if(currentMenu->menuitems[i].status != -1) {
            //
            // blinking letter for password menu
            //
            if(currentMenu == &PasswordDef && gametic & 4 && i == itemOn) {
                continue;
            }

            if(!currentMenu->smallfont) {
                rcolor fontcolor = MENUCOLORRED;

                if(itemSelected == i) {
                    fontcolor += D_RGBA(0, 128, 8, 0);
                }

                Draw_BigText(x, y, fontcolor, currentMenu->menuitems[i].name);
            }
            else {
                rcolor color = MENUCOLORWHITE;

                if(itemSelected == i) {
                    color = D_RGBA(255, 255, 0, menualphacolor);
                }

                //
                // tint the non-bindable key items to a shade of red
                //
                if(currentMenu == &ControlsDef) {
                    if(i >= (NUM_CONTROL_ITEMS - NUM_NONBINDABLE_ITEMS)) {
                        color = D_RGBA(255, 192, 192, menualphacolor);
                    }
                }

                Draw_Text(
                    x,
                    y,
                    color,
                    currentMenu->scale,
                    false,
                    currentMenu->menuitems[i].name
                );

                //
                // nasty hack to re-set the scale after a drawtext call
                //
                if(currentMenu->scale != 1) {
                    GL_SetOrthoScale(currentMenu->scale);
                }
            }
        }
        //
        // if menu item is static but has text, then display it as gray text
        // used for subcategories
        //
        else if(strlen(currentMenu->menuitems[i].name)) {
            if(!currentMenu->smallfont) {
                Draw_BigText(
                    -1,
                    y,
                    MENUCOLORWHITE,
                    currentMenu->menuitems[i].name
                );
            }
            else {
                int strwidth = M_StringWidth(currentMenu->menuitems[i].name);

                Draw_Text(
                    ((int)(160.0f / currentMenu->scale) - (strwidth / 2)),
                    y,
                    D_RGBA(255, 0, 0, menualphacolor),
                    currentMenu->scale,
                    false,
                    currentMenu->menuitems[i].name
                );

                //
                // nasty hack to re-set the scale after a drawtext call
                //
                if(currentMenu->scale != 1) {
                    GL_SetOrthoScale(currentMenu->scale);
                }
            }
        }

        if(currentMenu != &PasswordDef) {
            y += height;
        }
    }

    //
    // display indicators that the user can scroll farther up or down
    //
    if(currentMenu->numpageitems != -1) {
        if(currentMenu->menupageoffset) {
            //up arrow
            Draw_BigText(currentMenu->x, currentMenu->y - 24, MENUCOLORWHITE, "/u More...");
        }

        if(currentMenu->menupageoffset + currentMenu->numpageitems < currentMenu->numitems) {
            //down arrow
            Draw_BigText(currentMenu->x, (currentMenu->y - 2 + (currentMenu->numpageitems-1) * height) + 24,
                         MENUCOLORWHITE, "/d More...");
        }
    }

    //
    // draw password cursor
    //
    if(currentMenu == &PasswordDef) {
        Draw_BigText((currentMenu->x + ((itemOn & 7) * height)) - 4,
                     currentMenu->y + ((int)(itemOn / 8) * height) + 3, MENUCOLORWHITE, "/b");
    }
    else {
        // DRAW SKULL
        if(!currentMenu->smallfont) {
            int offset = 0;

            if(currentMenu->textonly) {
                x += SKULLXTEXTOFF;
            }
            else {
                x += SKULLXOFF;
            }

            if(itemOn) {
                for(i = itemOn; i > 0; i--) {
                    if(currentMenu->menuitems[i].status == -3) {
                        offset++;
                    }
                }
            }

            M_DrawMenuSkull(x, currentMenu->y - 5 + ((itemOn - currentMenu->menupageoffset) - offset) * height);
        }
        //
        // draw arrow cursor
        //
        else {
            Draw_BigText(x - 12,
                         currentMenu->y - 4 + (itemOn - currentMenu->menupageoffset) * height,
                         MENUCOLORWHITE, "/l");
        }
    }

    if(currentMenu->scale != 1) {
        GL_SetOrthoScale(1.0f);
    }

#ifdef _USE_XINPUT  // XINPUT
    if(xgamepad.connected && currentMenu != &MainDef) {
        GL_SetOrthoScale(0.75f);
        if(currentMenu == &PasswordDef) {
            M_DrawXInputButton(4, 271, XINPUT_GAMEPAD_B);
            Draw_Text(22, 276, MENUCOLORWHITE, 0.75f, false, "Change");
        }

        GL_SetOrthoScale(0.75f);
        M_DrawXInputButton(4, 287, XINPUT_GAMEPAD_A);
        Draw_Text(22, 292, MENUCOLORWHITE, 0.75f, false, "Select");

        if(currentMenu != &PauseDef) {
            GL_SetOrthoScale(0.75f);
            M_DrawXInputButton(5, 303, XINPUT_GAMEPAD_START);
            Draw_Text(22, 308, MENUCOLORWHITE, 0.75f, false, "Return");
        }

        GL_SetOrthoScale(1);
    }
#endif

    M_DrawCursor(mouse_x, mouse_y);
}


//
// M_ClearMenus
//

void M_ClearMenus(void) {
    if(!allowclearmenu) {
        return;
	}

    menufadefunc = NULL;
    nextmenu = NULL;
    menualphacolor = 0xff;
    menuactive = 0;

    S_ResumeSound();
}

//
// M_NextMenu
//

static void M_NextMenu(void) {
    currentMenu = nextmenu;
    itemOn = currentMenu->lastOn;
    menualphacolor = 0xff;
    alphaprevmenu = false;
    menufadefunc = NULL;
    nextmenu = NULL;
}


//
// M_SetupNextMenu
//

void M_SetupNextMenu(menu_t *menudef) {
    if(newmenu) {
        menufadefunc = M_NextMenu;
    }
    else {
        menufadefunc = M_MenuFadeOut;
    }

    alphaprevmenu = false;
    nextmenu = menudef;
    newmenu = false;
}


//
// M_MenuFadeIn
//

void M_MenuFadeIn(void) {
    int fadetime = (int)(m_menufadetime + 20.0f);

    if((menualphacolor + fadetime) < 0xff) {
        menualphacolor += fadetime;
    }
    else {
        menualphacolor = 0xff;
        alphaprevmenu = false;
        menufadefunc = NULL;
        nextmenu = NULL;
    }
}


//
// M_MenuFadeOut
//

void M_MenuFadeOut(void) {
    int fadetime = (int)(m_menufadetime + 20.0f);

    if(menualphacolor > fadetime) {
        menualphacolor -= fadetime;
    }
    else {
        menualphacolor = 0;

        if(alphaprevmenu == false) {
            currentMenu = nextmenu;
            itemOn = currentMenu->lastOn;
        }
        else {
            currentMenu = currentMenu->prevMenu;
            itemOn = currentMenu->lastOn;
        }

        menufadefunc = M_MenuFadeIn;
    }
}


//
// M_Ticker
//

extern BoolProperty p_features;

void M_Ticker(void) {
    mainmenuactive = (currentMenu == &MainDef) ? true : false;

    if((currentMenu == &MainDef ||
            currentMenu == &PauseDef) && usergame && demoplayback) {
        menuactive = 0;
        return;
    }
    if(!usergame) {
        OptionsDef.prevMenu = &MainDef;
        LoadDef.prevMenu = &MainDef;
        SaveDef.prevMenu = &MainDef;
    }
    else {
        OptionsDef.prevMenu = &PauseDef;
        LoadDef.prevMenu = &PauseDef;
        SaveDef.prevMenu = &PauseDef;
    }

    //
    // hidden features menu
    //
    if(currentMenu == &PauseDef) {
        currentMenu->menuitems[pause_features].status = p_features ? 1 : -3;
    }

    //
    // hidden hardcore difficulty option
    //
    if(currentMenu == &NewDef) {
        currentMenu->menuitems[nightmare].status = p_features ? 1 : -3;
    }

#ifdef _USE_XINPUT  // XINPUT
    //
    // hide mouse menu if xbox 360 controller is plugged in
    //
    if(currentMenu == &ControlMenuDef) {
        currentMenu->menuitems[controls_gamepad].status = xgamepad.connected ? 1 : -3;
    }
#endif

    //
    // hide anisotropic option if not supported on video card
    //
    if(!GLAD_GL_EXT_texture_filter_anisotropic) {
        VideoMenu[anisotropic].status = -3;
    }

    // auto-adjust itemOn and page offset if the first menu item is being used as a header
    if(currentMenu->menuitems[0].status == -1 &&
            strlen(currentMenu->menuitems[0].name)) {
        // bump page offset up
        if(itemOn == 1) {
            currentMenu->menupageoffset = 0;
        }

        // bump the cursor down
        if(itemOn <= 0) {
            itemOn = 1;
        }
    }

    if(menufadefunc) {
        menufadefunc();
    }

    // auto adjust page offset for long menu items
    if(currentMenu->numpageitems != -1) {
        if(itemOn >= (currentMenu->numpageitems + currentMenu->menupageoffset)) {
            currentMenu->menupageoffset = (itemOn + 1) - currentMenu->numpageitems;

            if(currentMenu->menupageoffset >= currentMenu->numitems) {
                currentMenu->menupageoffset = currentMenu->numitems;
            }
        }
        else if(itemOn < currentMenu->menupageoffset) {
            currentMenu->menupageoffset = itemOn;

            if(currentMenu->menupageoffset < 0) {
                currentMenu->menupageoffset = 0;
            }
        }
    }

    if(--skullAnimCounter <= 0) {
        whichSkull++;
        skullAnimCounter = 4;
    }

    if(thermowait != 0 && currentMenu->menuitems[itemOn].status == 3 &&
            currentMenu->menuitems[itemOn].routine) {
        currentMenu->menuitems[itemOn].routine(thermowait == -1 ? 1 : 0);
    }
}


//
// M_Init
//

void M_Init(void) {
    int i = 0;

    currentMenu = &MainDef;
    menuactive = 0;
    itemOn = currentMenu->lastOn;
    itemSelected = -1;
    whichSkull = 0;
    skullAnimCounter = 4;
    quickSaveSlot = -1;
    menufadefunc = NULL;
    nextmenu = NULL;
    newmenu = false;

    for(i = 0; i < NUM_CONTROL_ITEMS; i++) {
        ControlsItem[i].alphaKey = 0;
        dmemset(ControlsItem[i].name, 0, 64);
        ControlsItem[i].routine = NULL;
        ControlsItem[i].status = 1;
    }

    // setup password menu

    for(i = 0; i < 32; i++) {
        PasswordMenu[i].status = 1;
        PasswordMenu[i].name[0] = passwordChar[i];
        PasswordMenu[i].routine = NULL;
        PasswordMenu[i].alphaKey = (char)passwordChar[i];
    }

    dmemset(passwordData, 0xff, 16);

    MainDef.y += 8;
    NewDef.prevMenu = &MainDef;

    // setup region menu

    if(wad::have_lump("BLUDA0")) {
        m_regionblood = 0;
        RegionMenu[region_blood].status = 1;
    }

    if(!wad::have_lump("JPMSG01")) {
        st_regionmsg = false;
        RegionMenu[region_lang].status = 1;
    }

    if(!wad::have_lump("PLLEGAL") && !wad::have_lump("JPLEGAL")) {
        p_regionmode = 0;
        RegionMenu[region_mode].status = 1;
    }

    M_InitShiftXForm();
}
