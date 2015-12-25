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


#ifndef __P_LOCAL__
#define __P_LOCAL__

#include "doomdef.h"
#include "d_player.h"
#include "w_wad.h"
#include "info.h"
#include "m_menu.h"
#include "t_bsp.h"

#define FLOATSPEED        (FRACUNIT*4)


#define MAXHEALTH        100
#define VIEWHEIGHT        (56*FRACUNIT)    //villsa: changed from 41 to 56

// mapblocks are used to check movement
// against lines and things
#define MAPBLOCKUNITS    128
#define MAPBLOCKSIZE    (MAPBLOCKUNITS*FRACUNIT)
#define MAPBLOCKSHIFT    (FRACBITS+7)
#define MAPBMASK        (MAPBLOCKSIZE-1)
#define MAPBTOFRAC        (MAPBLOCKSHIFT-FRACBITS)


// player radius for movement checking
#define PLAYERRADIUS    19*FRACUNIT

// MAXRADIUS is for precalculated sector block boxes
// the spider demon is larger,
// but we do not have any moving sectors nearby
#define MAXRADIUS        32*FRACUNIT

#define GRAVITY        FRACUNIT
#define MAXMOVE        (16*FRACUNIT)           // [d64] changed from 30 to 16
#define STOPSPEED   0x1000
#define FRICTION    0xd200

#define USERANGE        (64*FRACUNIT)
#define MELEERANGE        (80*FRACUNIT)       // [d64] changed from 64 to 80
#define ATTACKRANGE     (16*64*FRACUNIT)
#define MISSILERANGE    (32*64*FRACUNIT)
#define LASERRANGE        (4096*FRACUNIT)
#define LASERAIMHEIGHT    (40*FRACUNIT)
#define LASERDISTANCE    (30*FRACUNIT)

// follow a player
#define    BASETHRESHOLD         90



//
// P_TICK
//

// both the head and tail of the thinker list
extern    thinker_t    thinkercap;
extern    mobj_t        mobjhead;

void P_InitThinkers(void);
void P_AddThinker(thinker_t* thinker);
void P_RemoveThinker(thinker_t* thinker);
void P_LinkMobj(mobj_t* mobj);
void P_UnlinkMobj(mobj_t* mobj);

extern angle_t frame_angle;
extern angle_t frame_pitch;
extern fixed_t frame_viewx;
extern fixed_t frame_viewy;
extern fixed_t frame_viewz;


//
// P_PSPR
//
struct laser_s;
typedef struct laser_s {
    fixed_t         x1;
    fixed_t         y1;
    fixed_t         z1;
    fixed_t         x2;
    fixed_t         y2;
    fixed_t         z2;
    fixed_t         slopex;
    fixed_t         slopey;
    fixed_t         slopez;
    fixed_t         dist;
    fixed_t         distmax;
    mobj_t*         marker;
    struct laser_s* next;
    angle_t         angle;
} laser_t;

typedef struct {
    thinker_t   thinker;
    mobj_t*     dest;
    laser_t*    laser;
} laserthinker_t;

void P_SetupPsprites(player_t* curplayer);
void P_MovePsprites(player_t* curplayer);
void P_DropWeapon(player_t* player);
void T_LaserThinker(laserthinker_t *laser);


//
// P_USER
//
void    P_PlayerThink(player_t* player);
void    P_Thrust(player_t* player, angle_t angle, fixed_t move);
angle_t P_AdjustAngle(angle_t angle, angle_t newangle, int threshold);
void    P_SetStaticCamera(player_t* player);
void    P_SetFollowCamera(player_t* player);
void    P_ClearUserCamera(player_t* player);


//
// P_MOBJ
//
#define ONFLOORZ        D_MININT
#define ONCEILINGZ      D_MAXINT

extern mapthing_t*  spawnlist;
extern int          numspawnlist;

mobj_t*     P_SpawnMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type);
void        P_SafeRemoveMobj(mobj_t* mobj);
void        P_RemoveMobj(mobj_t* th);
void        P_SpawnPlayer(mapthing_t* mthing);
void        P_SetTarget(mobj_t **mop, mobj_t *targ);
dboolean    P_SetMobjState(mobj_t* mobj, statenum_t state);
void        P_MobjThinker(mobj_t* mobj);
dboolean    P_OnMobjZ(mobj_t* mobj);
void        P_NightmareRespawn(mobj_t* mobj);
void        P_RespawnSpecials(mobj_t* special);
void        P_SpawnPuff(fixed_t x, fixed_t y, fixed_t z);
void        P_SpawnBlood(fixed_t x, fixed_t y, fixed_t z, int damage);
void        P_SpawnPlayerMissile(mobj_t* source, mobjtype_t type);
void        P_FadeMobj(mobj_t* mobj, int amount, int alpha, int flags);
int         EV_SpawnMobjTemplate(line_t* line);
int         EV_FadeOutMobj(line_t* line);
void        P_SpawnDartMissile(int tid, int type, mobj_t *target);
mobj_t*     P_SpawnMissile(mobj_t* source, mobj_t* dest, mobjtype_t type,
                           fixed_t xoffs, fixed_t yoffs, fixed_t heightoffs, dboolean aim);

//
// P_ENEMY
//
void P_NoiseAlert(mobj_t* target, mobj_t* emmiter);


//
// P_MAPUTL
//
typedef struct {
    fixed_t    x;
    fixed_t    y;
    fixed_t    dx;
    fixed_t    dy;

} divline_t;

typedef struct {
    fixed_t    frac;        // along trace line
    dboolean    isaline;
    union {
        mobj_t*    thing;
        line_t*    line;
    }            d;
} intercept_t;

#define MAXINTERCEPTS    128

extern intercept_t    intercepts[MAXINTERCEPTS];
extern intercept_t*    intercept_p;

typedef dboolean(*traverser_t)(intercept_t *in);

fixed_t P_AproxDistance(fixed_t dx, fixed_t dy);
int     P_PointOnLineSide(fixed_t x, fixed_t y, line_t* line);
int     P_PointOnDivlineSide(fixed_t x, fixed_t y, divline_t* line);
void     P_MakeDivline(line_t* li, divline_t* dl);
fixed_t P_InterceptVector(divline_t* v2, divline_t* v1);
void    P_GetIntersectPoint(fixed_t *s1, fixed_t *s2, fixed_t *x, fixed_t *y);
int     P_BoxOnLineSide(fixed_t* tmbox, line_t* ld);

extern fixed_t        opentop;
extern fixed_t         openbottom;
extern fixed_t        openrange;
extern fixed_t        lowfloor;

void         P_LineOpening(line_t* linedef);
dboolean    P_BlockLinesIterator(int x, int y, dboolean(*func)(line_t*));
dboolean    P_BlockThingsIterator(int x, int y, dboolean(*func)(mobj_t*));

#define PT_ADDLINES        1
#define PT_ADDTHINGS    2
#define PT_EARLYOUT        4

extern divline_t    trace;

dboolean P_PathTraverse(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2, int    flags, dboolean(*trav)(intercept_t *));
void    P_UnsetThingPosition(mobj_t* thing);
void    P_SetThingPosition(mobj_t* thing);


//
// P_MAP
//

// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
extern dboolean     floatok;
extern fixed_t      tmfloorz;
extern fixed_t      tmceilingz;
extern line_t*      tmhitline;

dboolean    P_CheckPosition(mobj_t *thing, fixed_t x, fixed_t y);
dboolean    P_TryMove(mobj_t* thing, fixed_t x, fixed_t y);
dboolean    P_PlayerMove(mobj_t* thing, fixed_t x, fixed_t y);
dboolean    P_TeleportMove(mobj_t* thing, fixed_t x, fixed_t y);
void        P_SlideMove(mobj_t* mo);
dboolean    P_CheckSight(mobj_t* t1, mobj_t* t2);
void        P_ScanSights(void);
dboolean    P_UseLines(player_t* player, dboolean showcontext);
dboolean    P_ChangeSector(sector_t* sector, dboolean crunch);
mobj_t*     P_CheckOnMobj(mobj_t *thing);
void        P_CheckChaseCamPosition(mobj_t* target, mobj_t* camera, fixed_t x, fixed_t y);

#define MAXSPECIALCROSS 64

extern mobj_t*  linetarget;    // who got hit (or NULL)
extern mobj_t*  blockthing;
extern fixed_t  aimfrac;
extern line_t*  spechit[MAXSPECIALCROSS];
extern int      numspechit;

fixed_t P_AimLineAttack(mobj_t*    t1, angle_t angle, fixed_t zheight, fixed_t distance);
void    P_LineAttack(mobj_t* t1,angle_t angle, fixed_t distance, fixed_t slope, int damage);
void    P_RadiusAttack(mobj_t* spot, mobj_t* source, int damage);



//
// P_SETUP
//
extern byte*        rejectmatrix;    // for fast sight rejection
extern short*        blockmaplump;    // offsets in blockmap are from here
extern short*        blockmap;
extern int            bmapwidth;
extern int            bmapheight;    // in mapblocks
extern fixed_t        bmaporgx;
extern fixed_t        bmaporgy;    // origin of block map
extern mobj_t**        blocklinks;    // for thing chains



//
// P_INTER
//
extern int        maxammo[NUMAMMO];
extern int        clipammo[NUMAMMO];

extern int infraredFactor;

void P_TouchSpecialThing(mobj_t* special, mobj_t* toucher);
void P_DamageMobj(mobj_t* target, mobj_t* inflictor, mobj_t* source, int damage);


//
// P_SPEC
//
#include "p_spec.h"


#endif    // __P_LOCAL__


