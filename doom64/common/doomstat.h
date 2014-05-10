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


#ifndef __D_STATE__
#define __D_STATE__

// We need globally shared data structures,
//  for defining the global state variables.
#include "doomdata.h"
#include "d_net.h"
#include "t_bsp.h"

// We need the playr data structure as well.
#include "d_player.h"

#ifdef __GNUG__
#pragma interface
#endif



// ------------------------
// Command line parameters.
//
extern  dboolean    nomonsters;     // checkparm of -nomonsters
extern  dboolean    respawnparm;    // checkparm of -respawn
extern  dboolean    respawnitem;
extern  dboolean    fastparm;       // checkparm of -fast
extern  dboolean    nolights;
extern  dboolean    devparm;        // DEBUG: launched with -devparm


// -------------------------------------------
// Selected skill type, map etc.
//

// Defaults for menu, methinks.
extern  skill_t     startskill;
extern  int         startmap;

extern  dboolean    autostart;

// Selected by user.
extern  skill_t     gameskill;
extern  int         gamemap;
extern  int         nextmap;

// Nightmare mode flag, single player.
extern  dboolean    respawnmonsters;
extern  dboolean    respawnspecials;

// Netgame? Only true if >1 player.
extern  dboolean    netgame;
extern  dboolean    netcheat;
extern  dboolean    netkill;

extern  int         gameflags;
extern  int         compatflags;

// Flag: true only if started as net deathmatch.
// An enum might handle altdeath/cooperative better.
extern int          deathmatch;

extern int          video_width;
extern int          video_height;
extern float        video_ratio;
extern int          window_focused;


// -------------------------
// Status flags for refresh.
//

// Depending on view size - no status bar?
// Note that there is no way to disable the
//  status bar explicitely.
extern  dboolean    statusbaractive;

extern  dboolean    automapactive;      // In AutoMap mode?
extern  dboolean    menuactive;         // Menu overlayed?
extern  dboolean    allowmenu;          // Allow menu interaction?
extern  dboolean    mainmenuactive;
extern  dboolean    allowclearmenu;
extern  dboolean    paused;             // Game Pause?

extern  dboolean    InWindow;
extern  dboolean    setWindow;
extern  int         ViewHeight;
extern  int         ViewWidth;

extern int          validcount;

extern int          numvertexes;
extern vertex_t     *vertexes;
extern int          numsegs;
extern seg_t        *segs;
extern int          numsectors;
extern sector_t     *sectors;
extern int          numsubsectors;
extern subsector_t  *subsectors;
extern int          numnodes;
extern node_t       *nodes;
extern int          numleafs;
extern leaf_t       *leafs;
extern int          numsides;
extern side_t       *sides;
extern int          numlines;
extern line_t       *lines;
extern light_t      *lights;
extern int          numlights;
extern macroinfo_t  macros;

// This one is related to the 3-screen display mode.
// ANG90 = left side, ANG270 = right
//extern  int    viewangleoffset;

// 1'st Player taking events, and displaying.

extern int          displayplayer;
extern int          consoleplayer;

// -------------------------------------
// Scores, rating.
// Statistics on a given map, for intermission.
//
extern  int         totalkills;
extern  int         totalitems;
extern  int         totalsecret;

// Timer, for scores.
extern  int         basetic;
extern  int         leveltime;      // tics in game play for par



// --------------------------------------
// DEMO playback/recording related stuff.
// No demo, there is a human player in charge?
// Disable save/end game?
extern  dboolean    usergame;
extern  dboolean    rundemo4;

//?
extern  dboolean    demoplayback;
extern  dboolean    demorecording;

// Quit after playing a demo from cmdline.
extern  dboolean    singledemo;

extern  gamestate_t gamestate;



//
// MAPINFO
//

typedef struct {
    char        mapname[64];
    int         mapid;
    int         music;
    int         type;
    int         cluster;
    int         exitdelay;
    dboolean    nointermission;
    dboolean    clearchts;
    dboolean    forcegodmode;
    dboolean    contmusexit;
    int         oldcollision;
    int         allowjump;
    int         allowfreelook;
} mapdef_t;

typedef struct {
    int         id;
    int         music;
    dboolean    enteronly;
    short       pic_x;
    short       pic_y;
    dboolean    nointermission;
    dboolean    scrolltextend;
    char        text[512];
    char        pic[9];
} clusterdef_t;


//-----------------------------
// Internal parameters, fixed.
// These are set by the engine, and not changed
//  according to user inputs. Partly load from
//  WAD, partly set at startup time.



extern  int         gametic;


// Bookkeeping on players - state.
extern  player_t    players[MAXPLAYERS];

// Alive? Disconnected?
extern  dboolean    playeringame[MAXPLAYERS];


// Player spawn spots for deathmatch.
#define MAX_DM_STARTS   10

extern  mapthing_t  deathmatchstarts[MAX_DM_STARTS];
extern  mapthing_t* deathmatch_p;

// Player spawn spots.
extern  mapthing_t  playerstarts[MAXPLAYERS];


// LUT of ammunition limits for each kind.
// This doubles with BackPack powerup item.
extern  int         maxammo[NUMAMMO];





//-----------------------------------------
// Internal parameters, used for engine.
//

// File handling stuff.
extern  char        basedefault[1024];
extern  FILE*       debugfile;

// if true, load all graphics at level load
extern  dboolean    precache;

#define MAXSENSITIVITY    32

extern  int         bodyqueslot;

// Netgame stuff (buffers and pointers, i.e. indices).

//moved doomcom and netbuffer into i_net.h so can't accidentaly clobber in split-screen mode

//don't need to know about localtics, probably broken to to splitscreen anyway...
//extern  ticcmd_t  localcmds[BACKUPTICS];
extern  int         rndindex;

extern  int         maketic;
extern  int         nettics[MAXNETNODES];

extern ticcmd_t     netcmds[MAXPLAYERS][BACKUPTICS];
extern  int         ticdup;
extern  int         extratics;

#endif
