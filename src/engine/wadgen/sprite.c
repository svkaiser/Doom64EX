// Emacs style mode select       -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: sprite.c 1096 2012-03-31 18:28:01Z svkaiser $
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
// DESCRIPTION: Sprite extracting/conversion mechanics.
//                              A majority of the application's workload is spent here...
//
//-----------------------------------------------------------------------------

#include <memory.h>

#include "wadgen.h"
#include "sprite.h"
#include "rom.h"
#include "wad.h"

d64ExSpriteLump_t exSpriteLump[MAX_SPRITES];
d64PaletteLump_t d64PaletteLump[24];
int extPalLumpCount = 0;
static int spriteIndexCount = 0;
int spriteExCount = 0;
int hudSpriteExCount = 0;

void Sprite_GetPalette(d64RawSprite_t * d64Spr, dPalette_t * palette, int lump);
void Sprite_GetPaletteLump(dPalette_t * palette, int lump);

// I was surprised to find that some of the GFX (overlay graphic) lumps are stored
// under the sprite format, which I call them hud gfx.
// I was even more surprised that the SPACE GFX is using the Doom1/2 playpal palette :)
// The following below are lumps that needs to be parsed as sprites, but converted to
// the PC GFX format later on..

static const romLumpSpecial_t HudSprNames[] = {
	{"SFONT", "EPJX"}, {"STATUS", "EPJX"}, {"SPACE", "EPJX"}, {"MOUNTA",
								   "EPJX"},
	{"MOUNTB", "EPJX"}, {"MOUNTC", "EPJX"}, {"JPMSG01", "PJ"}, {"JPMSG02",
								    "PJ"},
	{"JPMSG03", "PJ"}, {"JPMSG04", "PJ"}, {"JPMSG05", "PJ"}, {"JPMSG06",
								  "PJ"},
	{"JPMSG07", "PJ"}, {"JPMSG08", "PJ"}, {"JPMSG09", "PJ"}, {"JPMSG10",
								  "PJ"},
	{"JPMSG11", "PJ"}, {"JPMSG12", "PJ"}, {"JPMSG13", "PJ"}, {"JPMSG14",
								  "PJ"},
	{"JPMSG15", "PJ"}, {"JPMSG16", "PJ"}, {"JPMSG18", "PJ"}, {"JPMSG19",
								  "PJ"},
	{"JPMSG20", "PJ"}, {"JPMSG21", "PJ"}, {"JPMSG22", "PJ"}, {"JPMSG23",
								  "PJ"},
	{"JPMSG24", "PJ"}, {"JPMSG25", "PJ"}, {"JPMSG26", "PJ"}, {"JPMSG27",
								  "PJ"},
	{"JPMSG28", "PJ"}, {"JPMSG29", "PJ"}, {"JPMSG30", "PJ"}, {"JPMSG31",
								  "PJ"},
	{"JPMSG32", "PJ"}, {"JPMSG33", "PJ"}, {"JPMSG34", "PJ"}, {"JPMSG35",
								  "PJ"},
	{"JPMSG36", "PJ"}, {"JPMSG37", "PJ"}, {"JPMSG38", "PJ"}, {"JPMSG39",
								  "PJ"},
	{"JPMSG40", "PJ"}, {"JPMSG41", "PJ"}, {"JPMSG42", "PJ"}, {"JPMSG43",
								  "PJ"},
	{"JPMSG44", "PJ"}, {"JPMSG45", "PJ"}, {NULL, NULL}
};

static const romLumpSpecial_t BloodSprites[] = {
	{"RBLDA0", "P"}, {"RBLDB0", "P"}, {"RBLDC0", "P"}, {"RBLDD0", "P"},
	{"GBLDA0", "J"}, {"GBLDB0", "J"}, {"GBLDC0", "J"}, {"GBLDD0", "J"},
	{"BLUDA0", "EX"}, {"BLUDB0", "EX",}, {"BLUDC0", "EX"}, {"BLUDD0", "EX"},
	{NULL, NULL}
};

//**************************************************************
//**************************************************************
//      Sprite_Convert
//
//      N64 Doom64's sprite format is probably the most confusing
//      data to handle (also refer to Sprite.h for the N64 Sprite struct)
//      
//      Here is how this works:
//
//      There are two types of sprites. Compressed and Uncompressed.
//      Compressed sprites will ALWAYS be 16 colors, no more, no less.
//      Sprites at 16 colors are easily compressed by merging two bytes
//      into one byte (similar to the bitmap 4bit format). The palette
//      is always stored at the end of the sprite lump.
//
//      Uncompressed sprites will always be at 256 colors and will more
//      likely to rely on an external palette lump. If the lump is outside
//      the sprite markers, then that palette is stored in the sprite lump.
//
//      The most confusing part is how to work with the multiple tiles
//      that is divided in the sprite. Converting a single tiled sprite
//      is easy; unfortunetly, it becomes a lot more confusing when dealing
//      with multiple tiled sprites.
//
//      The reason for this is because usually on every 4th or 8th row
//      of the sprite's height, the bytes are flipped, meaning one half of
//      the row is at the beginning, while the first half is at the end. This
//      requires to write the last half of the row first and then write the other
//      half. This is a little hard to explain unfortunetly.
//**************************************************************
//**************************************************************

void Sprite_Convert(int lump)
{
	d64RawSprite_t *rs = NULL;
	d64ExSpriteLump_t *exs = NULL;
	byte *buffer = NULL;
	byte *readBuf = NULL;
	int w = 0;
	int h = 0;
	int pos = 0;
	int oldw = 0;
	int inv = 0;
	int id = 0;
	int i = 0;
	int tmp = 0;
	byte palID[3];

	// Must be a marker.. skip
	if (romWadFile.lump[lump].size == 0)
		return;

	// Check if this lump is a palette
	strncpy(palID, romWadFile.lump[lump].name, 3);
	if (palID[0] & 0x80)
		palID[0] = palID[0] - 0x80;

	if (palID[0] == 'P' && palID[1] == 'A' && palID[2] == 'L') {
		Sprite_GetPaletteLump(d64PaletteLump[extPalLumpCount].
				      extPalLumps, lump);
		strncpy(d64PaletteLump[extPalLumpCount].name,
			romWadFile.lump[lump].name, 8);
		if (d64PaletteLump[extPalLumpCount].name[0] & 0x80)
			d64PaletteLump[extPalLumpCount].name[0] -= (char)0x80;

		extPalLumpCount++;
		return;
	}

	exs = &exSpriteLump[spriteIndexCount++];

	// Setup the n64 raw sprite
	rs = (d64RawSprite_t *) romWadFile.lumpcache[lump];
	readBuf =
	    (byte *) (romWadFile.lumpcache[lump] + sizeof(d64RawSprite_t));

	oldw = rs->width = _SWAP16(rs->width);
	rs->height = _SWAP16(rs->height);
	rs->xoffs = _SWAP16(rs->xoffs);
	rs->yoffs = _SWAP16(rs->yoffs);
	rs->tiles = _SWAP16(rs->tiles);
	rs->compressed = _SWAP16(rs->compressed);
	rs->cmpsize = _SWAP16(rs->cmpsize);
	rs->tileheight = _SWAP16(rs->tileheight);

	// Here's where things gets confusing. If the sprite is compressed
	// then its width is padded by 16, while the uncompressed sprite has the width
	// padded by 8. On the N64, its a lot easier (and faster) to render a sprite in dimentions
	// of 8 or 16. Tile height pieces are always in multiples of 4
	rs->compressed == -1 ? (_PAD8(rs->width)) : (_PAD16(rs->width));

	buffer = (byte *) Mem_Alloc(sizeof(byte) * (rs->width * rs->height));
	exs->data = (byte *) Mem_Alloc(sizeof(byte) * (rs->width * rs->height) +
				       (rs->compressed ==
					-1 ? 0 : (sizeof(dPalette_t) * 256)));

	if (rs->compressed == -1) {
		for (i = 0; i < (rs->height * rs->width); i++)
			buffer[i] = readBuf[i];
	} else {
		// Decompress the sprite by taking one byte and turning it into two values
		for (tmp = 0, i = 0; i < (rs->width * rs->height) / 2; i++) {
			buffer[tmp++] = (readBuf[i] >> 4);
			buffer[tmp++] = (readBuf[i] & 0xf);
		}
	}

	memset(exs->palette, 0, sizeof(dPalette_t) * 256);
	Sprite_GetPalette(rs, exs->palette, lump);

	exs->lumpRef = lump;
	exs->sprite.width = rs->width;
	exs->sprite.height = rs->height;
	exs->sprite.offsetx = rs->xoffs;
	exs->sprite.offsety = rs->yoffs;

	if (!INSPRITELIST(lump)) {
		// Must be a hud sprite..
		exs->sprite.useExtPal = 1;
		exs->size = (rs->width * rs->height);
	} else {
		exs->sprite.useExtPal = rs->compressed == -1 ? 1 : 0;
		exs->size =
		    rs->compressed ==
		    -1 ? (rs->width * rs->height) : ((rs->width * rs->height) /
						     2);
	}

	// All gun sprites should be after this lump
	if (lump > Wad_GetLumpNum("RECTO0")) {
		// Doom64 has different xy offsets for the gun sprites than than of the PC
		// Regardless, move the xy offsets to a PC value which is by subtracting the x by 160 and
		// the y by 208, which should position the sprites in the center of the screen.
		exs->sprite.offsetx -= 160;
		exs->sprite.offsety -= 208;
	}

	tmp = 0;

	for (h = 0; h < rs->height; h++, id++) {
		// Reset the check for flipped rows if its beginning on a new tile
		if (id == rs->tileheight) {
			id = 0;
			inv = 0;
		}
		// This will handle the row flipping issue found on most multi tiled sprites..
		if (inv) {
			if (rs->compressed == -1) {
				for (w = 0; w < rs->width; w += 8) {
					for (pos = 4; pos < 8; pos++)
						exs->data[tmp++] =
						    buffer[(h * rs->width) + w +
							   pos];

					for (pos = 0; pos < 4; pos++)
						exs->data[tmp++] =
						    buffer[(h * rs->width) + w +
							   pos];
				}
			} else {
				for (w = 0; w < rs->width; w += 16) {
					for (pos = 8; pos < 16; pos++)
						exs->data[tmp++] =
						    buffer[(h * rs->width) + w +
							   pos];

					for (pos = 0; pos < 8; pos++)
						exs->data[tmp++] =
						    buffer[(h * rs->width) + w +
							   pos];
				}
			}
		} else		// Copy the sprite rows normally
			for (w = 0; w < rs->width; w++)
				exs->data[tmp++] = buffer[(h * rs->width) + w];

		inv ^= 1;
	}

	// Recompress the sprite for Doom64 EX.
	if ((!exs->sprite.useExtPal) && INSPRITELIST(lump)) {
		tmp = 0;
		memset(buffer, 0, rs->width * rs->height);
		memcpy(buffer, exs->data, rs->width * rs->height);
		memset(exs->data, 0, rs->width * rs->height);

		for (exs->data, i = 0; i < rs->width * rs->height; i += 2)
			exs->data[tmp++] = ((buffer[i + 1] << 4) | buffer[i]);

		for (i = 0; i < 16; i++) {
			exs->data[tmp++] = exs->palette[i].r;
			exs->data[tmp++] = exs->palette[i].g;
			exs->data[tmp++] = exs->palette[i].b;
			exs->data[tmp++] = exs->palette[i].a;
		}

		// hack to force first palette entry for key cards to be black
		if (Wad_GetLumpNum("RKEYA0") == lump ||
		    Wad_GetLumpNum("YKEYA0") == lump ||
		    Wad_GetLumpNum("BKEYA0") == lump) {
			exs->palette[0].r = 0;
			exs->palette[0].g = 0;
			exs->palette[0].b = 0;
		}
	}
}

//**************************************************************
//**************************************************************
//      Sprite_GetPalette
//
//      First decides if the sprite is compressed, or not. If not
//      compressed then start converting 256 palette indexes, else
//      just convert 16 colors.
//      
//      Returns if the sprite is within the S_START
//      and S_END markers and is not compressed, which means this
//      sprite uses an external palettel lump.
//**************************************************************
//**************************************************************

void Sprite_GetPalette(d64RawSprite_t * d64Spr, dPalette_t * palette, int lump)
{
	word *data;

	if (d64Spr->compressed == -1 && INSPRITELIST(lump))
		return;

	data =
	    (word *) (romWadFile.lumpcache[lump] +
		      (d64Spr->compressed ==
		       -1 ? (d64Spr->width *
			     d64Spr->height) : (d64Spr->cmpsize))
		      + sizeof(d64RawSprite_t));
	WGen_ConvertN64Pal(palette, data,
			   d64Spr->compressed == -1 ? 256 : CMPPALCOUNT);
}

//**************************************************************
//**************************************************************
//      Sprite_GetPaletteLump
//
//      Palette lumps are checked, which is used to convert them into
//      a standard palette format. Palette lumps can be indentified
//      as PALXXXX#, where XXXX is the sprite's name, and # is the number.
//      Some sprites can have multiple external palettes. The Imp and
//      Nightmare Imp are prime examples.
//**************************************************************
//**************************************************************

void Sprite_GetPaletteLump(dPalette_t * palette, int lump)
{
	word *data;

	data = (word *) (romWadFile.lumpcache[lump] + 8);
	WGen_ConvertN64Pal(palette, data, 256);
}

//**************************************************************
//**************************************************************
//      Sprite_Setup
//
//      First scans all lumps between the S_START and S_END markers
//      and converts them. Second it checks for all GFX lumps with
//      the sprite format and converts those as well but later
//      converts them to GFX during outputing the wad.
//**************************************************************
//**************************************************************

void Sprite_Setup(void)
{
	int l = 0;
	int i = 0;

#if 0
	// Rename blood sprites..
	if (RomFile.header.CountryID != 'E') {
		for (i = 0; BloodSprites[i].name; i++) {
			if (!Rom_VerifyRomCode(&BloodSprites[i]))
				continue;

			l = Wad_GetLumpNum(BloodSprites[i].name);

			strncpy(romWadFile.lump[l].name, "BLUDA0", 8);
			strncpy(romWadFile.lump[l + 1].name, "BLUDB0", 8);
			strncpy(romWadFile.lump[l + 2].name, "BLUDC0", 8);
			strncpy(romWadFile.lump[l + 3].name, "BLUDD0", 8);

			romWadFile.lump[l].name[0] |= 0x80;
			romWadFile.lump[l + 1].name[0] |= 0x80;
			romWadFile.lump[l + 2].name[0] |= 0x80;
			romWadFile.lump[l + 3].name[0] |= 0x80;

			break;
		}
	}
#endif

	i = l = 0;

	// Get sprites
	for (l = Wad_GetLumpNum("S_START") + 1; l < Wad_GetLumpNum("S_END");
	     l++) {
		romWadFile.lumpcache[l] =
		    Wad_GetLump(romWadFile.lump[l].name, false);
		Sprite_Convert(l);

		WGen_UpdateProgress("Converting Sprites...");
	}

	spriteExCount = spriteIndexCount;

	// Get Hud Sprites
	for (i = 0; HudSprNames[i].name; i++) {
		if (!Rom_VerifyRomCode(&HudSprNames[i]))
			continue;

		l = Wad_GetLumpNum(HudSprNames[i].name);
		romWadFile.lumpcache[l] =
		    Wad_GetLump(romWadFile.lump[l].name, false);
		Sprite_Convert(l);

		WGen_UpdateProgress("Converting Hud Sprites...");
	}

	hudSpriteExCount = spriteIndexCount - spriteExCount;
}
