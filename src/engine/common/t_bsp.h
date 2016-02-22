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

#ifndef T_BSP_H
#define T_BSP_H

#include "m_fixed.h"
#include "d_think.h"
#include "p_mobj.h"

//
// INTERNAL MAP TYPES
//  used by play and refresh
//

//
// Your plain vanilla vertex.
// Note: transformed values not buffered locally,
//  like some DOOM-alikes ("wt", "WebView") did.
//
typedef struct {
    fixed_t     x;
    fixed_t     y;

    // info for occlusion
    int         validcount;
    angle_t     clipspan;

} vertex_t;


// Forward of LineDefs, for Sectors.
struct line_s;

// Each sector has a degenmobj_t in its center
//  for sound origin purposes.
// I suppose this does not handle sound from
//  moving objects (doppler), because
//  position is prolly just buffered, not
//  updated.
typedef struct {
    fixed_t        x;
    fixed_t        y;
    fixed_t        z;

} degenmobj_t;

typedef struct {
    fixed_t a;
    fixed_t b;
    fixed_t c;
    fixed_t d;
} plane_t;

//
// The SECTORS record, at runtime.
// Stores things/mobjs.
//
typedef    struct {
    fixed_t         floorheight;
    fixed_t         ceilingheight;
    word            floorpic;
    word            ceilingpic;
    short            lightlevel;
    short           special;
    short           tag;

    // [d64] color indexes references for the lights lump
    short           colors[5];

    // [d64] special flags for sector
    word            flags;

    // [d64] x/y offsets for scrolling flats
    int             xoffset;
    int             yoffset;

    // 0 = untraversed, 1,2 = sndlines -1
    int             soundtraversed;

    // thing that made a sound (or null)
    mobj_t*         soundtarget;

    // mapblock bounding box for height changes
    int             blockbox[4];

    // origin for any sounds played by the sector
    degenmobj_t     soundorg;

    // if == validcount, already checked
    int             validcount;

    // list of mobjs in sector
    mobj_t*         thinglist;

    // thinker_t for reversable actions
    void*           specialdata;

    int             linecount;
    struct line_s** lines;    // [linecount] size

    // [kex] stuff that happens in between tics
    fixed_t         frame_z1[2];
    fixed_t         frame_z2[2];

    // [kex] plane/normal info for ceiling and floor
    plane_t         ceilingplane;
    plane_t         floorplane;

} sector_t;




//
// The SideDef.
//

typedef struct {
    // add this to the calculated texture column
    fixed_t    textureoffset;

    // add this to the calculated texture top
    fixed_t    rowoffset;

    // Texture indices.
    // We do not maintain names here.
    short    toptexture;
    short    bottomtexture;
    short    midtexture;

    // Sector the SideDef is facing.
    sector_t*    sector;

} side_t;



//
// Move clipping aid for LineDefs.
//
typedef enum {
    ST_HORIZONTAL,
    ST_VERTICAL,
    ST_POSITIVE,
    ST_NEGATIVE

} slopetype_t;


typedef struct line_s {
    // Vertices, from v1 to v2.
    vertex_t*       v1;
    vertex_t*       v2;

    // Precalculated v2 - v1 for side checking.
    fixed_t         dx;
    fixed_t         dy;

    int             flags;
    short           special;
    short           tag;

    // Visual appearance: SideDefs.
    //  sidenum[1] will be -1 if one sided
    word            sidenum[2];

    // Neat. Another bounding box, for the extent
    //  of the LineDef.
    fixed_t         bbox[4];

    // To aid move clipping.
    slopetype_t     slopetype;

    // Front and back sector.
    // Note: redundant? Can be retrieved from SideDefs.
    sector_t*       frontsector;
    sector_t*       backsector;

    // if == validcount, already checked
    int             validcount;

    // thinker_t for reversable actions
    void*           specialdata;

    angle_t         angle;

} line_t;




//
// A SubSector.
// References a Sector.
// Basically, this is a list of LineSegs,
//  indicating the visible walls that define
//  (all or some) sides of a convex BSP leaf.
//
// Also includes boundary vertices when gl-friendly nodes used
typedef struct subsector_s {
    sector_t*   sector;
    word        numlines;
    word        firstline;
    word        numleafs;
    word        leaf;
} subsector_t;

//
// Sprites are patches with a special naming convention
//  so they can be recognized by R_InitSprites.
// The base name is NNNNFx or NNNNFxFx, with
//  x indicating the rotation, x = 0, 1-7.
// The sprite and frame specified by a thing_t
//  is range checked at run time.
// A sprite is a patch_t that is assumed to represent
//  a three dimensional object and may have multiple
//  rotations pre drawn.
// Horizontal flipping is used to save space,
//  thus NNNNF2F5 defines a mirrored patch.
// Some sprites will only have one picture used
// for all views: NNNNF0
//
typedef struct {
    // If false use 0 for any position.
    // Note: as eight entries are available,
    //  we might as well insert the same name eight times.
    dboolean    rotate;

    // Lump to use for view angles 0-7.
    short    lump[8];

    // Flip bit (1 = flip) to use for view angles 0-7.
    byte    flip[8];

} spriteframe_t;



//
// A sprite definition:
//  a number of animation frames.
//
typedef struct {
    int             numframes;
    spriteframe_t*  spriteframes;

} spritedef_t;


//
// The LineSeg.
//
typedef struct {
    vertex_t*       v1;
    vertex_t*       v2;

    fixed_t         offset;

    angle_t         angle;

    side_t*         sidedef;
    line_t*            linedef;

    // Sector references.
    // Could be retrieved from linedef, too.
    // backsector is NULL for one sided lines
    sector_t*        frontsector;
    sector_t*       backsector;

    fixed_t         length;

} seg_t;



//
// BSP node.
//
typedef struct {
    // Partition line.
    fixed_t    x;
    fixed_t    y;
    fixed_t    dx;
    fixed_t    dy;

    // Bounding box for each child.
    fixed_t    bbox[2][4];

    // If NF_SUBSECTOR its a subsector.
    unsigned short children[2];

} node_t;

//
// LEAFS structure
//
typedef struct {
    vertex_t    *vertex;
    seg_t        *seg;
} leaf_t;


//
// Light Data
//
typedef struct {
    byte base_r;
    byte base_g;
    byte base_b;
    byte active_r;
    byte active_g;
    byte active_b;
    byte r;
    byte g;
    byte b;
    short tag;
} light_t;

//
// Macros
//
typedef struct {
    short id;
    short tag;
    short special;
} macrodata_t;

typedef struct {
    short count;
    macrodata_t* data;
} macrodef_t;

typedef struct {
    short macrocount;
    short specialcount;
    macrodef_t* def;
} macroinfo_t;

#endif