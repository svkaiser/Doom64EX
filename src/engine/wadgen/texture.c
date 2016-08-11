// Emacs style mode select       -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: texture.c 1096 2012-03-31 18:28:01Z svkaiser $
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
// DESCRIPTION: Texture parsing and converting
//
//-----------------------------------------------------------------------------

#include <string.h>

#include "wadgen.h"
#include "wad.h"
#include "texture.h"

static d64RawTexture_t d64RomTexture[MAXTEXTURES];
d64ExTexture_t d64ExTexture[MAXTEXTURES];

//**************************************************************
//**************************************************************
//      Texture_CreateRomLump
//      Bytes are read differently on the N64, so flip the nibbles per byte
//      Depending on the width, each number of rows would have each four bytes
//      flipped like the sprites.
//**************************************************************
//**************************************************************

void Texture_CreateRomLump(d64RawTexture_t * tex, cache data)
{
	int size = 0;
	int palsize = 0;
	int mask = 0;
	int i = 0;
	int *tmpSrc;

	memset(tex, 0, sizeof(d64RawTexture_t));
	memcpy(&tex->header, data, sizeof(d64RawTextureHeader_t));

	tex->header.hshift = _SWAP16(tex->header.hshift);
	tex->header.id = _SWAP16(tex->header.id);
	tex->header.numpal = _SWAP16(tex->header.numpal);
	tex->header.wshift = _SWAP16(tex->header.wshift);

	size = ((1 << tex->header.wshift) * (1 << tex->header.hshift)) >> 1;
	mask = (1 << tex->header.wshift) / 8;

	_PAD8(size);

	tex->data = (byte *) Mem_Alloc(size);
	memcpy(tex->data, data + (sizeof(d64RawTextureHeader_t)), size);

	palsize = sizeof(short) * (NUMTEXPALETTES * tex->header.numpal);
	tex->palette = (word *) Mem_Alloc(palsize);
	memcpy(tex->palette, data + (sizeof(d64RawTextureHeader_t) + size),
	       palsize);

	// Flip nibbles per byte
	for (i = 0; i < size; i++) {
		byte tmp = tex->data[i];

		tex->data[i] = (tmp >> 4);
		tex->data[i] |= ((tmp & 0xf) << 4);
	}

	tmpSrc = (int *)(tex->data);

	// Flip each sets of dwords based on texture width
	for (i = 0; i < size / 4; i += 2) {
		int x1;
		int x2;

		if (i & mask) {
			x1 = *(int *)(tmpSrc + i);
			x2 = *(int *)(tmpSrc + i + 1);

			*(int *)(tmpSrc + i) = x2;
			*(int *)(tmpSrc + i + 1) = x1;
		}
	}
}

//**************************************************************
//**************************************************************
//      Texture_CreateExLump
//      Setup a new Doom64 EX texture
//**************************************************************
//**************************************************************

void Texture_CreateExLump(d64ExTexture_t * pcTex, d64RawTexture_t * romTex)
{
	int i = 0;

	memset(pcTex, 0, sizeof(d64ExTexture_t));

	pcTex->header.width = (1 << romTex->header.wshift);
	pcTex->header.height = (1 << romTex->header.hshift);
	pcTex->header.compressed = 1;
	pcTex->header.numpal = romTex->header.numpal;
	pcTex->size = (pcTex->header.width * pcTex->header.height);

	pcTex->data =
	    (byte *) Mem_Alloc(pcTex->header.width * pcTex->header.height);

	for (i = 0; i < pcTex->size / 2; i++)
	{
		byte tmp = romTex->data[i];
		pcTex->data[2*i+1] = tmp >> 4;
		pcTex->data[2*i] = tmp & 0xf;
	}

	for (i = 0; i < romTex->header.numpal; i++)
		WGen_ConvertN64Pal(pcTex->palette[i],
				   romTex->palette + (i * NUMTEXPALETTES),
				   NUMTEXPALETTES);
}

//**************************************************************
//**************************************************************
//      Texture_Setup
//**************************************************************
//**************************************************************

void Texture_Setup(void)
{
	int i = 0;
	int pos = 0;

	for (i = Wad_GetLumpNum("T_START") + 1; i < Wad_GetLumpNum("T_END");
	     i++) {
		romWadFile.lumpcache[i] =
		    Wad_GetLump(romWadFile.lump[i].name, true);

		Texture_CreateRomLump(&d64RomTexture[pos],
				      romWadFile.lumpcache[i]);
		Texture_CreateExLump(&d64ExTexture[pos], &d64RomTexture[pos]);

		// Masked textures are identified by having its first palette color all 0s, but
		// the nukage textures appear to have 0 RGB for its first palette color.. which
		// causes them to be masked or see-through. This hack will prevent that..

		if (i == Wad_GetLumpNum("SLIMEA")
		    || i == Wad_GetLumpNum("SLIMEB")) {
			d64ExTexture[pos].palette[0][0].r =
			    d64ExTexture[pos].palette[0][0].g =
			    d64ExTexture[pos].palette[0][0].b = 1;
		}

		d64ExTexture[pos++].lumpRef = i;

		WGen_UpdateProgress("Converting Textures...");
	}
}
