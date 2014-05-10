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

#ifndef __DOOMDATA__
#define __DOOMDATA__

// The most basic types we use, portability.
#include "doomtype.h"

// Some global defines, that configure the game.
#include "doomdef.h"
#include "m_fixed.h"


//
// Map level types.
// The following data structures define the persistent format
// used in the lumps of the WAD files.
//

// Lump order in a map WAD: each map needs a couple of lumps
// to provide a complete scene geometry description.
enum {
    ML_LABEL,           // A separator, name, ExMx or MAPxx
    ML_THINGS,          // Monsters, items..
    ML_LINEDEFS,        // LineDefs, from editing
    ML_SIDEDEFS,        // SideDefs, from editing
    ML_VERTEXES,        // Vertices, edited and BSP splits generated
    ML_SEGS,            // LineSegs, from LineDefs split by BSP
    ML_SSECTORS,        // SubSectors, list of LineSegs
    ML_NODES,           // BSP nodes
    ML_SECTORS,         // Sectors, from editing
    ML_REJECT,          // LUT, sector-sector visibility
    ML_BLOCKMAP,        // LUT, motion clipping, walls/grid element
    ML_LEAFS,           // LUT for true subsector rendering
    ML_LIGHTS,          // Color light table, from editing
    ML_MACROS           // Linedef macros, from editing
};

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

// A single Vertex.
typedef struct {
    fixed_t    x;
    fixed_t    y;
} PACKEDATTR mapvertex_t;

typedef struct {
    byte r;
    byte g;
    byte b;
    byte a;
    short tag;
} PACKEDATTR maplights_t;


// A SideDef, defining the visual appearance of a wall,
// by setting textures and offsets.
typedef struct {
    short    textureoffset;
    short    rowoffset;
    word    toptexture;
    word    bottomtexture;
    word    midtexture;
    short    sector;    // Front sector, towards viewer.
} PACKEDATTR mapsidedef_t;



// A LineDef, as used for editing, and as input
// to the BSP builder.
typedef struct {
    word    v1;
    word    v2;
    int     flags;
    short   special;
    short   tag;
    //
    // support more than 32768 sidedefs
    // use the unsigned value and special case the -1
    // sidenum[1] will be -1 (NO_INDEX) if one sided
    //
    word    sidenum[2]; // sidenum[1] will be -1 if one sided
} PACKEDATTR maplinedef_t;

#define NO_SIDE_INDEX   ((word)-1)

//
// LineDef attributes.
//

#define ML_BLOCKING             1               // Solid, is an obstacle.
#define ML_BLOCKMONSTERS        2               // Blocks monsters only.
#define ML_TWOSIDED             4               // Backside will not be present at all if not two sided.

// If a texture is pegged, the texture will have
// the end exposed to air held constant at the
// top or bottom of the texture (stairs or pulled
// down things) and will move with a height change
// of one of the neighbor sectors.
// Unpegged textures allways have the first row of
// the texture at the top pixel of the line for both
// top and bottom textures (use next to windows).

#define ML_DONTPEGTOP           8               // upper texture unpegged
#define ML_DONTPEGBOTTOM        16              // lower texture unpegged
#define ML_SECRET               32              // In AutoMap: don't map as two sided: IT'S A SECRET!
#define ML_SOUNDBLOCK           64              // Sound rendering: don't let sound cross two of these.
#define ML_DONTDRAW             128             // Don't draw on the automap at all.
#define ML_MAPPED               256             // Set if already seen, thus drawn in automap.
#define ML_DRAWMIDTEXTURE       0x200           // Draw middle texture on sidedef
#define ML_DONTOCCLUDE          0x400           // Don't add to occlusion buffer
#define ML_DONTPEGMID           0x800           // middle texture unpegged
#define ML_THINGTRIGGER         0x1000          // Line is triggered by dead thing (flagged as ondeathtrigger)
#define ML_SWITCHX02            0x2000          // Switch flag 1
#define ML_SWITCHX04            0x4000          // Switch flag 2
#define ML_SWITCHX08            0x8000          // Switch flag 3
#define ML_CHECKFLOORHEIGHT     0x10000         // if true then check the switch's floor height, else use the ceiling height
#define ML_SCROLLRIGHT          0x20000         // scroll texture to the right
#define ML_SCROLLLEFT           0x40000         // scroll texture to the left
#define ML_SCROLLUP             0x80000         // scroll texture up
#define ML_SCROLLDOWN           0x100000        // scroll texture down
#define ML_BLENDFULLTOP         0x200000        // do not extend blending for top texture
#define ML_BLENDFULLBOTTOM      0x400000        // do not extend blending for bottom texture
#define ML_BLENDING             0x800000        // use sector color blending (top/lower, ceiling, floor colors).
#define ML_TRIGGERFRONT         0x1000000       // can only trigger from the front of the line
#define ML_HIDEAUTOMAPTRIGGER   0x2000000       // don't display as yellow line special in automap
#define ML_INVERSEBLEND         0x4000000       // reverse the blending of the sector colors
#define ML_UNKNOWN8000000       0x8000000       // reserved
#define ML_UNKNOWN10000000      0x10000000      // reserved
#define ML_UNKNOWN20000000      0x20000000      // reserved
#define ML_HMIRROR              0x40000000      // horizontal mirror the texture
#define ML_VMIRROR              0x80000000      // vertical mirror the texture

//
// Special attributes.
//

#define MLU_MACRO               0x100            // line is set to be used as a macro
#define MLU_RED                 0x200            // requires red key
#define MLU_BLUE                0x400            // requires blue key
#define MLU_YELLOW              0x800            // requires yellow key
#define MLU_CROSS               0x1000            // must cross to trigger
#define MLU_SHOOT               0x2000            // must shoot the line to trigger
#define MLU_USE                 0x4000            // must press use on the line to trigger
#define MLU_REPEAT              0x8000            // line can be reactivated again

//
// Line masks
//

#define SWITCHMASK(x)           (x & 0x6000)
#define SPECIALMASK(x)          (x & 0x1FF)
#define MACROMASK(x)            (SPECIALMASK(x) - (x & MLU_MACRO))

// Sector definition, from editing.
typedef    struct {
    short    floorheight;
    short    ceilingheight;
    word    floorpic;
    word    ceilingpic;
    word    colors[5];
    short    special;
    short    tag;
    word    flags;
} PACKEDATTR mapsector_t;

//
// Sector Flags
//

#define MS_REVERB               1       // sounds are echoed in this sector
#define MS_REVERBHEAVY          2       // heavier echo effect
#define MS_LIQUIDFLOOR          4       // water effect (blitting two flats together)
#define MS_SYNCSPECIALS         8       // sync light special with multiple sectors
#define MS_SCROLLFAST           16      // faster ceiling/floor scrolling
#define MS_SECRET               32      // count secret when entering and display message
#define MS_DAMAGEX5             64      // damage player x5
#define MS_DAMAGEX10            128     // damage player x10
#define MS_DAMAGEX20            256     // damage player x20
#define MS_HIDESSECTOR          512     // hide subsectors in automap (textured mode)
#define MS_SCROLLCEILING        1024    // enable ceiling scrolling
#define MS_SCROLLFLOOR          2048    // enable floor scrolling
#define MS_SCROLLLEFT           4096    // scroll flat to the left
#define MS_SCROLLRIGHT          8192    // scroll flat to the right
#define MS_SCROLLUP             16384   // scroll flat to the north
#define MS_SCROLLDOWN           32768   // scroll flat to the south

// SubSector, as generated by BSP.
typedef struct {
    word    numsegs;
    word    firstseg;   // Index of first one, segs are stored sequentially.
} PACKEDATTR mapsubsector_t;


// LineSeg, generated by splitting LineDefs
// using partition lines selected by BSP builder.
typedef struct {
    word    v1;
    word    v2;
    short    angle;
    word    linedef;
    short    side;
    short    offset;
} PACKEDATTR mapseg_t;

// BSP node structure.

// Indicate a leaf.
#define    NF_SUBSECTOR    0x8000

typedef struct {
    // Partition line from (x,y) to x+dx,y+dy)
    short    x;
    short    y;
    short    dx;
    short    dy;

    // Bounding box for each child,
    // clip against view frustum.
    short    bbox[2][4];

    // If NF_SUBSECTOR its a subsector,
    // else it's a node of another subtree.
    word    children[2];

} PACKEDATTR mapnode_t;

// Thing definition, position, orientation and type,
// plus skill/visibility flags and attributes.
typedef struct {
    short    x;
    short    y;
    short    z;
    short    angle;
    short    type;
    short    options;
    short    tid;
} PACKEDATTR mapthing_t;

#ifdef _MSC_VER
#pragma pack(pop)
#endif


#endif            // __DOOMDATA__

