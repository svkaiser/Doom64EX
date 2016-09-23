// Emacs style mode select       -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: Gfx.c 1096 2012-03-31 18:28:01Z svkaiser $
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
// $Revision: 1096 $
// $Date: 2012-03-31 11:28:01 -0700 (Sat, 31 Mar 2012) $
//
// DESCRIPTION: GFX parsing and converting
//
//-----------------------------------------------------------------------------
#ifdef RCSID
static const char rcsid[] = "$Id: Gfx.c 1096 2012-03-31 18:28:01Z svkaiser $";
#endif

#include <string.h>

#include "wadgen.h"
#include "rom.h"
#include "wad.h"
#include "texture.h"
#include "gfx.h"

gfxEx_t gfxEx[MAXGFXEXITEMS];
gfxRom_t gfxRom[MAXGFXEXITEMS];

static const romLumpSpecial_t GfxNames[] = {
	{"SYMBOLS", "EPJX"}, {"USLEGAL", "EPJX"}, {"TITLE", "EPJX"}, {"EVIL",
								      "EPJX"},
	{"FIRE", "EPJX"}, {"CLOUD", "EPJX"}, {"IDCRED1", "EPJX"}, {"IDCRED2",
								   "EPJX"},
	{"WMSCRED1", "EPJX"}, {"WMSCRED2", "EPJX"}, {"FINAL", "EPJX"},
	    {"JPCPAK",
	     "J",},
	{"PLLEGAL", "PJX"}, {"JPLEGAL", "PJ"}, {"JAPFONT", "J"}, {NULL, NULL}
};

//**************************************************************
//**************************************************************
//      Gfx_CreateRomLump
//
//      Creates a gfx lump from the N64 rom. Width and height are padded
//**************************************************************
//**************************************************************

void Gfx_CreateRomLump(gfxRom_t * gfx, cache data)
{
	int32 size = 0;

	ZeroMemory(gfx, sizeof(gfxRom_t));
	memcpy(&gfx->header, data, sizeof(gfxRomHeader_t));

	gfx->header.width = _SWAP16(gfx->header.width);
	gfx->header.height = _SWAP16(gfx->header.height);
	gfx->header.compressed = _SWAP16(gfx->header.compressed);
	gfx->header.u2 = _SWAP16(gfx->header.u2);

	size = (gfx->header.width * gfx->header.height);
	_PAD8(size);

	gfx->data = malloc(size);

	memcpy(gfx->data, data + (sizeof(gfxRomHeader_t)), size);
	memcpy(&gfx->palette, data + (sizeof(gfxRomHeader_t) + size), 512);
}

//**************************************************************
//**************************************************************
//      Gfx_CreateRomFireLump
//**************************************************************
//**************************************************************

void Gfx_CreateRomFireLump(gfxRom_t * gfx, cache data)
{
	int32 size = 0;

	ZeroMemory(gfx, sizeof(gfxRom_t));
	memcpy(&gfx->header, data, sizeof(gfxRomHeader_t));

	gfx->header.width = _SWAP16(gfx->header.width);
	gfx->header.height = _SWAP16(gfx->header.height);
	gfx->header.compressed = _SWAP16(gfx->header.compressed);
	gfx->header.u2 = _SWAP16(gfx->header.u2);

	size = (gfx->header.width * gfx->header.height);
	_PAD8(size);

	gfx->data = malloc(size);

	memcpy(gfx->data, data + (sizeof(gfxRomHeader_t)), size);
}

//**************************************************************
//**************************************************************
//      Gfx_CreateEXLump
//
//      Creates a gfx lump for Doom64 EX. Width and height are padded
//**************************************************************
//**************************************************************

void Gfx_CreateEXLump(gfxEx_t * pcGfx, gfxRom_t * romGfx)
{
	int w = 0;
	int h = 0;

	ZeroMemory(pcGfx, sizeof(gfxEx_t));

	pcGfx->width = romGfx->header.width;
	pcGfx->height = romGfx->header.height;
	pcGfx->palPos = pcGfx->size = 0;

	_PAD4(pcGfx->width);
	_PAD4(pcGfx->height);

	pcGfx->data = malloc(pcGfx->width * pcGfx->height);
    ZeroMemory(pcGfx->data, pcGfx->width * pcGfx->height);

	for (h = 0; h < romGfx->header.height; h++) {
		for (w = 0; w < romGfx->header.width; w++)
			pcGfx->data[(h * pcGfx->width) + w] =
			    romGfx->data[(h * romGfx->header.width) + w];
	}

	WGen_ConvertN64Pal(pcGfx->palette, romGfx->palette, 256);
}

//**************************************************************
//**************************************************************
//      Gfx_CreateFirePalette
//**************************************************************
//**************************************************************

void Gfx_CreateFirePalette(gfxEx_t * pcGfx)
{
	int i = 0;
	int c = ((0xff << 24) | 0x000000);
	dPalette_t *pal = pcGfx->palette;

	memset(pal, 0, sizeof(dPalette_t) * 256);

	for (i = 0; i < 16; i++) {
		memcpy(pal, &c, sizeof(dPalette_t));
		c += 0x101010;

		pal += 16;
	}
}

//**************************************************************
//**************************************************************
//      Gfx_GetCloudLump
//
//      Somewhat similar to the texture format
//**************************************************************
//**************************************************************

void Gfx_GetCloudLump(gfxEx_t * pcGfx)
{
	int l = 0;
	int i = 0;
	int mask = 64 / 4;
	int *tmpSrc;

	l = Wad_GetLumpNum("CLOUD");
	romWadFile.lumpcache[l] = Wad_GetLump(romWadFile.lump[l].name, false);

	ZeroMemory(pcGfx, sizeof(gfxEx_t));

	pcGfx->width = 64;
	pcGfx->height = 64;
	pcGfx->data = malloc(4096);

	memcpy(pcGfx->data, romWadFile.lumpcache[l] + 8, 4096);

	tmpSrc = (int *)pcGfx->data;

	// Flip each sets of dwords based on texture width
	for (i = 0; i < 4096 / 4; i += 2) {
		int x1;
		int x2;

		if (i & mask) {
			x1 = *(int *)(tmpSrc + i);
			x2 = *(int *)(tmpSrc + i + 1);

			*(int *)(tmpSrc + i) = x2;
			*(int *)(tmpSrc + i + 1) = x1;
		}
	}

	WGen_ConvertN64Pal(pcGfx->palette,
			   (word *) (romWadFile.lumpcache[l] + 4104), 256);

	pcGfx->lumpRef = l;
}

//**************************************************************
//**************************************************************
//      Gfx_Setup
//**************************************************************
//**************************************************************

void Gfx_Setup(void)
{
	int l = 0;
	int i = 0;

	for (i = 0; i < MAXGFXEXITEMS; i++)
		gfxEx[i].data = NULL;

	// Get gfx
	for (i = 0; GfxNames[i].name; i++) {
		if (!Rom_VerifyRomCode(&GfxNames[i]))
			continue;

		if (!strncmp("CLOUD", GfxNames[i].name, 8))
			Gfx_GetCloudLump(&gfxEx[i]);
		else {
			l = Wad_GetLumpNum(GfxNames[i].name);
			romWadFile.lumpcache[l] =
			    Wad_GetLump(romWadFile.lump[l].name, false);

			if (!strncmp("FIRE", GfxNames[i].name, 8))
				Gfx_CreateRomFireLump(&gfxRom[i],
						      romWadFile.lumpcache[l]);
			else
				Gfx_CreateRomLump(&gfxRom[i],
						  romWadFile.lumpcache[l]);

			Gfx_CreateEXLump(&gfxEx[i], &gfxRom[i]);

			// fire lump has no palette, so create one
			if (!strncmp("FIRE", GfxNames[i].name, 8))
				Gfx_CreateFirePalette(&gfxEx[i]);

			gfxEx[i].lumpRef = l;	// Need to point where we originally got the gfx from
		}

		WGen_UpdateProgress("Converting GFX...");
	}
}
