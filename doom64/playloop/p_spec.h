// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 1997 Midway Home Entertainment, Inc
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


#ifndef __P_SPEC__
#define __P_SPEC__


//
// End-level timer (-TIMER option)
//
extern    dboolean levelTimer;
extern    int    levelTimeCount;

extern short globalint;

typedef struct {
    int         delay;
    char        name[9];
    int         frames;
    int         speed;
    dboolean    reverse;
    dboolean    palette;
} animdef_t;

extern int          numanimdef;
extern animdef_t*   animdefs;

void        P_InitPicAnims(void);       // at game start
void        P_SpawnSpecials(void);      // at map load
void        P_UpdateSpecials(void);     // every tic
int         P_DoSpecialLine(mobj_t* thing, line_t* line, int side);
void        P_AddSectorSpecial(sector_t* sector);
void        P_SpawnDelayTimer(line_t* line, void (*func)(void));

// when needed
dboolean    P_UseSpecialLine(mobj_t* thing, line_t* line, int side);
void        P_PlayerInSpecialSector(player_t* player);

int         twoSided(int sector, int line);
sector_t*   getSector(int currentSector, int line, int side);
side_t*     getSide(int currentSector, int line, int side);
sector_t*   getNextSector(line_t* line, sector_t* sec);

fixed_t     P_FindLowestFloorSurrounding(sector_t* sec);
fixed_t     P_FindHighestFloorSurrounding(sector_t* sec);
fixed_t     P_FindNextHighestFloor(sector_t* sec, int currentheight);
fixed_t     P_FindLowestCeilingSurrounding(sector_t* sec);
fixed_t     P_FindHighestCeilingSurrounding(sector_t* sec);
int         P_FindSectorFromLineTag(line_t* line, int start);
dboolean    P_ActivateLineByTag(int tag, mobj_t* activator);


//
// SPECIAL
//

int EV_DoFloorAndCeiling(line_t * line, dboolean fast, dboolean elevatorOrSplit);


//
// P_LIGHTS
//

typedef struct {
    thinker_t    thinker;
    sector_t*    sector;
    int          count;
    int          special;
} fireflicker_t;

typedef struct {
    thinker_t    thinker;
    sector_t*    sector;
    int          count;
    int          special;
} lightflash_t;

typedef struct {
    thinker_t    thinker;
    sector_t*    sector;
    int          count;
    int          maxlight;
    int          darktime;
    int          brighttime;
    int          special;
} strobe_t;

typedef struct {
    thinker_t    thinker;
    sector_t*    sector;
    int          type;
    int          count;
    int          minlight;
    int          direction;
    int          maxlight;
    int          special;
} glow_t;

typedef struct {
    thinker_t    thinker;
    sector_t     *sector;
    sector_t     *headsector;
    int          count;
    int          start;
    int          index;
    int          special;
} sequenceGlow_t;

typedef struct {
    thinker_t    thinker;
    sector_t     *sector;
    thinker_t    *combiner;
    int          special;
    actionf_p1   func;
} combine_t;

typedef struct {
    thinker_t thinker;
    light_t *dest;
    light_t *src;
    int r;
    int g;
    int b;
    int inc;
} lightmorph_t;

#define GLOWSPEED           2
#define STROBEBRIGHT        1
#define SUPERFAST           10
#define FASTDARK            15
#define SLOWDARK            30
#define PULSENORMAL         0
#define PULSESLOW           1
#define PULSERANDOM         2

void        P_SpawnFireFlicker(sector_t* sector);
void        T_LightFlash(lightflash_t* flash);
void        P_SpawnLightFlash(sector_t* sector);
void        T_StrobeFlash(strobe_t* flash);
void        T_FireFlicker(fireflicker_t* flick);
void        P_UpdateLightThinker(light_t* destlight, light_t* srclight);
void        T_Sequence(sequenceGlow_t* seq);
void        P_SpawnStrobeFlash(sector_t* sector, int speed);
void        P_SpawnStrobeAltFlash(sector_t* sector, int speed);
void        EV_StartLightStrobing(line_t* line);
void        T_Glow(glow_t* g);
void        P_SpawnGlowingLight(sector_t* sector, byte type);
void        P_SpawnSequenceLight(sector_t* sector, dboolean first);
void        P_CombineLightSpecials(sector_t* sector);
void        T_Combine(combine_t *combine);
dboolean    P_ChangeLightByTag(int tag1, int tag2);
int         P_DoSectorLightChange(line_t* line, short tag);
void        P_FadeInBrightness(void);


typedef enum {
    top,
    middle,
    bottom
} bwhere_e;


typedef struct {
    line_t*     line;
    bwhere_e    where;
    int         btexture;
    int         btimer;
    mobj_t*     soundorg;
} button_t;




// max # of wall switches in a level
#define MAXSWITCHES       11

// 4 players, 4 buttons each at once, max.
#define MAXBUTTONS        16

// 1 second, in ticks.
#define BUTTONTIME      15

extern button_t    buttonlist[MAXBUTTONS];

void P_ChangeSwitchTexture(line_t* line, int useAgain);


//
// P_PLATS
//
typedef enum {
    up,
    down,
    waiting,
    in_stasis
} plat_e;



typedef enum {
    perpetualRaise,
    downWaitUpStay,
    raiseAndChange,
    raiseToNearestAndChange,
    blazeDWUS,
    upWaitDownStay,
    blazeUWDS,
    customDownUp,
    customDownUpFast,
    customUpDown,
    customUpDownFast
} plattype_e;



typedef struct {
    thinker_t    thinker;
    sector_t*    sector;
    fixed_t      speed;
    fixed_t      low;
    fixed_t      high;
    int          wait;
    int          count;
    plat_e       status;
    plat_e       oldstatus;
    dboolean     crush;
    int          tag;
    plattype_e   type;
} plat_t;



#define PLATWAIT        3
#define PLATSLOWSPEED   (2*FRACUNIT)
#define PLATSPEED       (8*FRACUNIT)
#define PLATBLAZESPEED  (16*FRACUNIT)
#define MAXPLATS        60


extern plat_t*    activeplats[MAXPLATS];

void    T_PlatRaise(plat_t*    plat);
int        EV_DoPlat(line_t* line, plattype_e type, int amount);
void    P_AddActivePlat(plat_t* plat);
void    P_RemoveActivePlat(plat_t* plat);
void    EV_StopPlat(line_t* line);
void    P_ActivateInStasis(int tag);


//
// P_DOORS
//

typedef enum {
    normal,
    close30ThenOpen,
    doorclose,
    dooropen,
    raiseIn5Mins,
    blazeRaise,
    blazeOpen,
    blazeClose
} vldoor_e;



typedef struct {
    thinker_t   thinker;
    vldoor_e    type;
    sector_t*   sector;
    fixed_t     topheight;
    fixed_t     bottomheight;
    fixed_t     initceiling;
    fixed_t     speed;

    // 1 = up, 0 = waiting at top, -1 = down
    int         direction;

    // tics to wait at the top
    int         topwait;

    // (keep in case a door going down is reset)
    // when it reaches 0, start going down
    int         topcountdown;

} vldoor_t;



#define VDOORSPEED        FRACUNIT*2
#define VDOORBLAZESPEED   FRACUNIT*8
#define VDOORWAIT         120

void    EV_VerticalDoor(line_t* line, mobj_t* thing);
int     EV_DoDoor(line_t* line, vldoor_e type);
void    T_VerticalDoor(vldoor_t* door);



//
// P_CEILNG
//
typedef enum {
    lowerToFloor,
    raiseToHighest,
    lowerAndCrush,
    crushAndRaise,
    fastCrushAndRaise,
    silentCrushAndRaise,
    customCeiling,
    crushAndRaiseOnce,
    customCeilingToHeight
} ceiling_e;



typedef struct {
    thinker_t    thinker;
    ceiling_e    type;
    sector_t*    sector;
    fixed_t      bottomheight;
    fixed_t      topheight;
    fixed_t      speed;
    dboolean     crush;
    int          direction;    // 1 = up, 0 = waiting, -1 = down
    int          tag;        // ID
    int          olddirection;
    dboolean     instant;
} ceiling_t;





#define CEILSPEED       FRACUNIT*2
#define CEILWAIT        150
#define MAXCEILINGS     30

extern ceiling_t*    activeceilings[MAXCEILINGS];

int        EV_DoCeiling(line_t* line, ceiling_e type, fixed_t speed);
void    T_MoveCeiling(ceiling_t* ceiling);
void    P_AddActiveCeiling(ceiling_t* c);
void    P_RemoveActiveCeiling(ceiling_t* c);
int        EV_CeilingCrushStop(line_t* line);
void    P_ActivateInStasisCeiling(line_t* line);


//
// P_FLOOR
//
typedef enum {
    lowerFloor,             // lower floor to highest surrounding floor
    lowerFloorToLowest,     // lower floor to lowest surrounding floor
    turboLower,             // lower floor to highest surrounding floor VERY FAST
    raiseFloor,             // raise floor to lowest surrounding CEILING
    raiseFloorToNearest,    // raise floor to next highest surrounding floor
    lowerAndChange,         // lower floor to lowest surrounding floor and change floorpic
    raiseFloor24,
    raiseFloor24AndChange,
    raiseFloorCrush,
    customFloor,
    customFloorToHeight
} floor_e;




typedef enum {
    build8, // slowly build by 8
    turbo16 // quickly build by 16
} stair_e;



typedef struct {
    thinker_t    thinker;
    floor_e      type;
    dboolean     crush;
    sector_t*    sector;
    int          direction;
    int          newspecial;
    short        texture;
    fixed_t      floordestheight;
    fixed_t      speed;
    dboolean     instant;
} floormove_t;

typedef struct {
    thinker_t    thinker;
    sector_t     *sector;
    fixed_t      ceildest;
    fixed_t      flrdest;
    int          ceildir;
    int          flrdir;
} splitmove_t;



#define FLOORSPEED FRACUNIT * 3

typedef enum {
    ok,
    crushed,
    pastdest,
    stop

} result_e;

result_e T_MovePlane(sector_t* sector, fixed_t speed, fixed_t dest,
                     dboolean crush, int floorOrCeiling, int direction);

int EV_BuildStairs(line_t* line, stair_e type);
int EV_DoFloor(line_t* line, floor_e floortype, fixed_t speed);
void T_MoveFloor(floormove_t* floor);
void T_MoveSplitPlane(splitmove_t *split);
int EV_SplitSector(line_t *line, dboolean sync);

//
// P_TELEPT
//
int EV_Teleport(line_t* line, int side, mobj_t* thing);
int EV_SilentTeleport(line_t* line, mobj_t* thing);



// Misc stuff

typedef struct {
    thinker_t thinker;
    int tics;
    void (*finishfunc)(void);
} delay_t;

typedef struct {
    thinker_t thinker;
    mobj_t* viewmobj;
    player_t* activator;
} aimcamera_t;

typedef struct {
    thinker_t thinker;
    fixed_t x;
    fixed_t y;
    fixed_t z;
    fixed_t slopex;
    fixed_t slopey;
    fixed_t slopez;
    player_t* player;
    int current;
    int tic;
} movecamera_t;

typedef struct {
    thinker_t thinker;
    mobj_t* mobj;
    int amount;
    int destAlpha;
    int flagReserve;
} mobjfade_t;

typedef struct {
    thinker_t thinker;
    int delay;
    int lifetime;
    int delaymax;
    mobj_t *mobj;
} mobjexp_t;

typedef struct {
    thinker_t thinker;
    int tics;
} quake_t;

typedef struct {
    thinker_t   thinker;
    fixed_t     x1;
    fixed_t     x2;
    fixed_t     y1;
    fixed_t     y2;
    fixed_t     z;
    int         flags;
    int         tic;
} tracedrawer_t;

//thinkers

void T_LightMorph(lightmorph_t* lt);
void T_CountdownTimer(delay_t* timer);
void T_MobjExplode(mobjexp_t *mexp);
void T_LookAtCamera(aimcamera_t* camera);
void T_MovingCamera(movecamera_t* camera);
void T_MobjFadeThinker(mobjfade_t* mobjfade);
void T_Quake(quake_t* quake);
void T_TraceDrawer(tracedrawer_t* tdrawer);

#endif
