// Emacs style mode select       -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: Level.c 1097 2012-04-01 22:24:04Z svkaiser $
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// $Author: svkaiser $
// $Revision: 1097 $
// $Date: 2012-04-01 15:24:04 -0700 (Sun, 01 Apr 2012) $
//
// DESCRIPTION: Level extraction routines
//
//-----------------------------------------------------------------------------

#include <string.h>

#include "wadgen.h"
#include "wad.h"
#include "level.h"

//#define DUMPMAPWAD
#define HASHINDICES

cache levelData[MAXLEVELWADS];
int levelSize[MAXLEVELWADS];

enum {
	ML_LABEL,		// A separator, name, MAPxx
	ML_THINGS,		// Monsters, items..
	ML_LINEDEFS,		// LineDefs, from editing
	ML_SIDEDEFS,		// SideDefs, from editing
	ML_VERTEXES,		// Vertices, edited and BSP splits generated
	ML_SEGS,		// LineSegs, from LineDefs split by BSP
	ML_SSECTORS,		// SubSectors, list of LineSegs
	ML_NODES,		// BSP nodes
	ML_SECTORS,		// Sectors, from editing
	ML_REJECT,		// LUT, sector-sector visibility
	ML_BLOCKMAP,		// LUT, motion clipping, walls/grid element
	ML_LEAFS,		// Data that guides Doom64 to auto-complete segs/subsectors into convex polygons
	ML_LIGHTS,		// Color light table, from editing
	ML_MACROS,		// Linedef macros, from editing
};

typedef struct {
	short textureoffset;
	short rowoffset;
	short toptexture;
	short bottomtexture;
	short midtexture;
	short sector;
} mapsidedef_t;

typedef struct {
	short floorheight;
	short ceilingheight;
	word floorpic;
	word ceilingpic;
	word colors[5];
	short special;
	short tag;
	word flags;
} mapsector_t;

//**************************************************************
//**************************************************************
//      Level_HashTextureName
//**************************************************************
//**************************************************************

static uint Level_HashTextureName(const char *str)
{
	uint hash = 1315423911;
	uint i = 0;

	for (i = 0; i < 8 && *str != '\0'; str++, i++)
		hash ^= ((hash << 5) + toupper((int)*str) + (hash >> 2));

	return hash % 65536;
}

//**************************************************************
//**************************************************************
//      Level_HashSidedefs
//**************************************************************
//**************************************************************

static void Level_HashSidedefs(cache data, int size)
{
	mapsidedef_t *sd = (mapsidedef_t *) data;
	int tstart = Wad_GetLumpNum("T_START") + 1;
	uint i;

	for (i = 0; i < (size / sizeof(mapsidedef_t)); i++, sd++) {
		char *name;

		name = romWadFile.lump[tstart + sd->bottomtexture].name;

		if (name[0] & 0x80)
			name[0] -= (char)0x80;

		sd->bottomtexture = Level_HashTextureName(name);

		name = romWadFile.lump[tstart + sd->midtexture].name;

		if (name[0] & 0x80)
			name[0] -= (char)0x80;

		sd->midtexture = Level_HashTextureName(name);

		name = romWadFile.lump[tstart + sd->toptexture].name;

		if (name[0] & 0x80)
			name[0] -= (char)0x80;

		sd->toptexture = Level_HashTextureName(name);
	}
}

//**************************************************************
//**************************************************************
//      Level_HashSectors
//**************************************************************
//**************************************************************

static void Level_HashSectors(cache data, int size)
{
	mapsector_t *ss = (mapsector_t *) data;
	int tstart = Wad_GetLumpNum("T_START") + 1;
	uint i;

	for (i = 0; i < (size / sizeof(mapsector_t)); i++, ss++) {
		char *name;

		name = romWadFile.lump[tstart + ss->ceilingpic].name;

		if (name[0] & 0x80)
			name[0] -= (char)0x80;

		ss->ceilingpic = Level_HashTextureName(name);

		name = romWadFile.lump[tstart + ss->floorpic].name;

		if (name[0] & 0x80)
			name[0] -= (char)0x80;

		ss->floorpic = Level_HashTextureName(name);
	}
}

//**************************************************************
//**************************************************************
//      Level_GetMapWad
//**************************************************************
//**************************************************************

void Level_GetMapWad(int num)
{
	char name[9];
	wadheader_t wad;
	lump_t *l = NULL;
	int lump = 0;
	int i = 0;
	byte *buffer;

	sprintf(name, "MAP%02d", num + 1);
	name[8] = 0;

	lump = Wad_GetLumpNum(name);
	romWadFile.lumpcache[lump] =
	    (byte*) Wad_GetLump(romWadFile.lump[lump].name, true);

	levelSize[num] = romWadFile.lump[lump].size;

	memcpy(&wad, romWadFile.lumpcache[lump], sizeof(wadheader_t));

	buffer = (byte*) malloc(levelSize[num]);
	memcpy(buffer, &wad, sizeof(wadheader_t));
	memcpy(buffer + sizeof(wadheader_t),
	       romWadFile.lumpcache[lump] + sizeof(wadheader_t),
	       (wad.lmpdirpos - sizeof(wadheader_t)) +
	       (sizeof(lump_t) * wad.lmpcount));

#ifdef HASHINDICES

	l = (lump_t *) (buffer + wad.lmpdirpos);
	Level_HashSidedefs(buffer + l[ML_SIDEDEFS].filepos,
			   l[ML_SIDEDEFS].size);
	Level_HashSectors(buffer + l[ML_SECTORS].filepos, l[ML_SECTORS].size);

#endif

	levelData[num] = buffer;
}

//**************************************************************
//**************************************************************
//      Level_Setup
//**************************************************************
//**************************************************************

void Level_Setup(void)
{
	int i;

	for (i = 0; i < MAXLEVELWADS; i++) {
		Level_GetMapWad(i);
		WGen_UpdateProgress("Decompressing MAP%02d...", i);
	}
}
