// Emacs style mode select       -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: wad.c 1150 2012-06-11 00:18:31Z svkaiser $
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
// $Revision: 1150 $
// $Date: 2012-06-10 17:18:31 -0700 (Sun, 10 Jun 2012) $
//
// DESCRIPTION: General Wad handling mechanics.
//                              Wad output and sprite/gfx/data writing also included
//
//-----------------------------------------------------------------------------
#ifdef RCSID
static const char rcsid[] = "$Id: wad.c 1150 2012-06-11 00:18:31Z svkaiser $";
#endif

#include "wadgen.h"
#include "rom.h"
#include "wad.h"
#include "files.h"
#include "deflate-N64.h"

#define ROM_IWADTITLE	0x44415749	// IWAD

wadFile_t romWadFile;
wadFile_t outWadFile;

cache romWadData;

typedef struct {
	uint position;
	char countryID;
	char version;
} iwadRomInfo_t;

static const iwadRomInfo_t IwadRomInfo[4] = {
	{0x63f60, 'P', 0},
	{0x64580, 'J', 0},
	{0x63d10, 'E', 0},
	{0x63dc0, 'E', 1}
};

char weaponName[4] = { 'N', 'U', 'L', 'L' };

cache png;
dPalette_t pngpal[256];
int pngsize;

//**************************************************************
//**************************************************************
//      Wad_Decompress
//
//      Based off of JaguarDoom's decompression algorithm.
//      This is a rather simple LZSS type algorithm that was used
//      on all lumps in JaguarDoom and PSXDoom.
//
//      Doom64 only uses this on the Sprites and GFX lumps, but uses
//      a more sophisticated algorithm for everything else, which is
//      why only the sprites and gfx are gathered from the rom as
//      I have yet to understand the new compression method used.
//**************************************************************
//**************************************************************

void Wad_Decompress(byte * input, byte * output)
{
	int getidbyte = 0;
	int len;
	int pos;
	int i;
	byte *source;
	int idbyte = 0;

	/*idbyte plays an important role, it specifies whenever something is compressed or
	   decompressed by shifting the bits and finding out if there is a 0 or 1. */
	while (1) {
		if (!getidbyte)
			idbyte = *input++;
		getidbyte = (getidbyte + 1) & 7;	/*assign a new idbyte every 8th loop */

		if (idbyte & 1) {
			/*begin decompressing and get position */
			pos = *input++ << 4;
			pos = pos | (*input >> 4);

			/*setup string */
			source = output - pos - 1;

			/*setup length */
			len = (*input++ & 0xf) + 1;
			if (len == 1)
				break;

			/*copy what bytes that have been outputed so far */
			for (i = 0; i < len; i++)
				*output++ = *source++;
		} else
			*output++ = *input++;	/*not compressed, just output the byte as is. */

		/*shift to next bit and begin the check at the beginning */
		idbyte = idbyte >> 1;

	}
}

//**************************************************************
//**************************************************************
//      Wad_CheckLump
//
//      Slightly different than Doom's function.
//      Checks for compressed lumps (first character of a lump name
//      will be incremented by 128, which should return true if & by 128)
//**************************************************************
//**************************************************************

int Wad_CheckLump(const char *name)
{
	char tempname[9] = { 0 };
	lump_t *lump_p;
	char first;

	strncpy(tempname, name, 8);
	strupr(tempname);

	first = tempname[0];

	lump_p = romWadFile.lump + romWadFile.header.lmpcount;

	while (lump_p-- != romWadFile.lump) {
		tempname[0] |= (lump_p->name[0] & 0x80);
		if (!strncmp(lump_p->name, tempname, 8))
			return lump_p - romWadFile.lump;
		tempname[0] = first;
	}

	return -1;
}

//**************************************************************
//**************************************************************
//      Wad_GetLumpNum
//**************************************************************
//**************************************************************

int Wad_GetLumpNum(const char *name)
{
	int i;

	i = Wad_CheckLump(name);

	if (i == -1)
		WGen_Complain("Wad_GetLumpNum: %s not found!", name);

	return i;
}

//**************************************************************
//**************************************************************
//      Wad_GetLump
//
//      Gets the data from a lump and caches it.
//**************************************************************
//**************************************************************

void *Wad_GetLump(char *name, bool dcmpType)
{
	byte *data;
	byte *dmpLmp;		/*Decompression buffer */
	int lump = Wad_GetLumpNum(name);

	data = (byte *) Mem_Alloc(romWadFile.lump[lump].size);
	memcpy(data, romWadData + romWadFile.lump[lump].filepos,
	       romWadFile.lump[lump].size);

	/*Parse compressed lump */
	if (romWadFile.lump[lump].name[0] & 0x80) {
		dmpLmp = (byte *) Mem_Alloc(romWadFile.lump[lump].size);
		if (dcmpType)
			Deflate_Decompress(data, dmpLmp);
		else
			Wad_Decompress(data, dmpLmp);

		memcpy(data, dmpLmp, romWadFile.lump[lump].size);
		Mem_Free((void **)&dmpLmp);

		return data;
	}

	if (data == NULL) {
		WGen_Complain
		    ("Wad_GetLump: Couldn't allocate %u bytes from %s\n",
		     romWadFile.lump[lump].size, romWadFile.lump[lump].name);
	}

	return data;
}

//**************************************************************
//**************************************************************
//      Wad_FindIwadLocation
//**************************************************************
//**************************************************************

static uint Wad_FindIwadLocation(void)
{
	int i = 0;

	for (i = 0; i < 4; i++) {
		if (IwadRomInfo[i].countryID == RomFile.header.CountryID) {
			if (IwadRomInfo[i].version != RomFile.header.VersionID)
				continue;

			return IwadRomInfo[i].position;
		}
	}

	WGen_Complain
	    ("Wad_FindIwadLocation: Unable to locate IWAD. Possible invalid ROM..");
	return 0;
}

//**************************************************************
//**************************************************************
//      Wad_CheckIwad
//
//      Verify the IWAD's location inside the N64 rom.
//**************************************************************
//**************************************************************

void Wad_CheckIwad(void)
{
	int *check;

	check = (int *)(RomFile.data + Wad_FindIwadLocation());
	if (*check != ROM_IWADTITLE)
		WGen_Complain("Rom_GetIwad: IWAD header not found..");
}

//**************************************************************
//**************************************************************
//      Wad_GetIwad
//
//      Begins caching the IWAD file from the rom
//**************************************************************
//**************************************************************

void Wad_GetIwad(void)
{
	Wad_CheckIwad();

	// Setup IWAD data
	romWadData = (byte *) (RomFile.data + Wad_FindIwadLocation());
	ZeroMemory(romWadFile.lump, sizeof(lump_t) * MAX_LUMPS);
	memcpy(&romWadFile.header, romWadData, sizeof(wadheader_t));
	romWadFile.size =
	    ((romWadFile.header.lmpcount * sizeof(lump_t)) +
	     romWadFile.header.lmpdirpos);
	memcpy(&romWadFile.lump, romWadData + romWadFile.header.lmpdirpos,
	       romWadFile.header.lmpcount * sizeof(lump_t));
}

//**************************************************************
//**************************************************************
//      Wad_CreateOutput
//
//      Create the header for the output wad
//**************************************************************
//**************************************************************

void Wad_CreateOutput(void)
{
	strncpy(outWadFile.header.id, "IWAD", 4);
	outWadFile.header.lmpdirpos = sizeof(wadheader_t);
	outWadFile.header.lmpcount = 0;
	outWadFile.size = sizeof(wadheader_t);

	ZeroMemory(outWadFile.lump, sizeof(lump_t) * MAX_LUMPS);
}

//**************************************************************
//**************************************************************
//      Wad_WriteOutput
//
//      Output wad is written to disk.
//**************************************************************
//**************************************************************

void Wad_WriteOutput(path outFile)
{
	int i = 0;
	int size = 0;
	int handle;

	handle = File_Open(outFile);

	File_Write(handle, &outWadFile.header, sizeof(wadheader_t));

	for (i = 0; i < outWadFile.header.lmpcount; i++) {
		size = outWadFile.lump[i].size;
		if (size) {
			_PAD4(size);
			File_Write(handle, outWadFile.lumpcache[i], size);
		}
	}

	for (i = 0; i < outWadFile.header.lmpcount; i++)
		File_Write(handle, &outWadFile.lump[i], sizeof(lump_t));

	File_Close(handle);
	File_SetReadOnly(outFile);
}

//**************************************************************
//**************************************************************
//      Wad_AddOutputLump
//
//      Allocates new space for the new lump to be added
//**************************************************************
//**************************************************************

void Wad_AddOutputLump(const char *name, int size, cache data)
{
	int padSize = 0;
	wadheader_t *header;
	lump_t *lump;

	header = &outWadFile.header;
	lump = &outWadFile.lump[header->lmpcount];

	//Update Lump
	lump->filepos = header->lmpdirpos;
	lump->size = size;

	strncpy(lump->name, name, 8);
	if (lump->name[0] & 0x80)
		lump->name[0] -= (char)0x80;

	WGen_UpdateProgress("Adding Lump: %s...", lump->name);

	if (size) {
		char *cache;

		padSize = size;
		_PAD4(padSize);

		//Allocate Cache
		cache = (byte *) Mem_Alloc(padSize);
		memcpy(cache, data, size);

		outWadFile.lumpcache[header->lmpcount] = cache;
	}

	WGen_AddDigest(lump->name, header->lmpcount, size);

	//Update header
	header->lmpcount++;
	header->lmpdirpos += padSize;
}

//**************************************************************
//**************************************************************
//      Wad_AddOutputSprite
//
//      Sets up the sprite lump to be written to the output wad
//**************************************************************
//**************************************************************

void Wad_AddOutputSprite(d64ExSpriteLump_t * sprite)
{
#ifdef USE_PNG
	int j = 0;

	// fetch external sprite palette
	if (sprite->sprite.useExtPal) {
		bool ok = false;
		char name[8];

		memcpy(name, romWadFile.lump[sprite->lumpRef].name, 8);

		// change first char to readable char
		if (name[0] & 0x80)
			name[0] -= (char)0x80;

		// fetch external palette. If none matches the sprite, then its probably a weapon palette
		for (j = 0; j < extPalLumpCount; j++) {
			// skip first three chars in d64PaletteLump so just the sprite name can be compared
			if (!strncmp
			    (name, (char *)(d64PaletteLump[j].name + 3), 4)) {
				ok = true;
				memcpy(pngpal, d64PaletteLump[j].extPalLumps,
				       sizeof(dPalette_t) * 256);
				break;
			}
		}

		if (!ok)	// fetch weapon palette
		{
			d64RawSprite_t *raw;

			// weapon palettes appears to be stored only in the first frame
			// preserve the current cached weapon palette if next sprite is the next weapon frame
			if (strncmp(weaponName, name, 4)) {
				raw =
				    (d64RawSprite_t *) romWadFile.
				    lumpcache[sprite->lumpRef];
				WGen_ConvertN64Pal(pngpal,
						   (word
						    *) (romWadFile.lumpcache
							[sprite->lumpRef] +
							(raw->width *
							 raw->height) + 16),
						   256);

				strncpy(weaponName, name, 4);
			}
		}
	}

	png = Png_Create(sprite->sprite.width, sprite->sprite.height, sprite->sprite.useExtPal ? 16 : 1,	// 256 colors if external
			 sprite->sprite.useExtPal ? pngpal : sprite->palette, sprite->sprite.useExtPal ? 8 : 4,	// 8 bit if external
			 sprite->data, sprite->lumpRef, &pngsize);

	Wad_AddOutputLump(romWadFile.lump[sprite->lumpRef].name, pngsize, png);
#else
	cache data;
	int size;
	char name[8];
	int pos = 0;

	size = (sizeof(d64ExSprite_t) + sprite->size);
	if (!sprite->sprite.useExtPal)
		size += (sizeof(dPalette_t) * CMPPALCOUNT);
	else if (sprite->sprite.useExtPal
		 && sprite->lumpRef > Wad_GetLumpNum("RECTO0")) {
		sprite->sprite.useExtPal = 2;
		size += (sizeof(dPalette_t) * 256);
	}

	strncpy(name, romWadFile.lump[sprite->lumpRef].name, 8);

	data = (byte *) Mem_Alloc(size);

	memcpy(data, &sprite->sprite, sizeof(d64ExSprite_t));
	pos += sizeof(d64ExSprite_t);

	memcpy((data + pos), sprite->data, sprite->size);
	pos += sprite->size;

	if (!sprite->sprite.useExtPal)
		memcpy((data + pos), sprite->palette,
		       sizeof(dPalette_t) * CMPPALCOUNT);
	else if (sprite->sprite.useExtPal
		 && sprite->lumpRef > Wad_GetLumpNum("RECTO0")) {
		d64RawSprite_t *raw;

		// weapon palettes appears to be stored only in the first frame
		// preserve the current cached weapon palette if next sprite is the next weapon frame
		if (strncmp(weaponName, name, 4)) {
			raw =
			    (d64RawSprite_t *) romWadFile.lumpcache[sprite->
								    lumpRef];
			WGen_ConvertN64Pal(pngpal,
					   (word
					    *) (romWadFile.lumpcache[sprite->
								     lumpRef] +
						(raw->width * raw->height) +
						16), 256);

			strncpy(weaponName, name, 4);
		}

		memcpy((data + pos), pngpal, sizeof(dPalette_t) * 256);
	}

	Wad_AddOutputLump(name, size, data);

	Mem_Free((void **)&data);

#endif
}

//**************************************************************
//**************************************************************
//      Wad_AddOutputTexture
//
//      Sets up the texture lump to be written to the output wad
//**************************************************************
//**************************************************************

void Wad_AddOutputTexture(d64ExTexture_t * tex)
{
#ifdef USE_PNG
	int j = 0;
	int i = 0;
	int pos = 0;

	// merge multiple texture palettes into one array
	for (j = 0; j < tex->header.numpal; j++) {
		for (i = 0; i < 16; i++) {
			pngpal[pos].r = tex->palette[j][i].r;
			pngpal[pos].g = tex->palette[j][i].g;
			pngpal[pos].b = tex->palette[j][i].b;
			pngpal[pos].a = tex->palette[j][i].a;
			pos++;
		}
	}

	png = Png_Create(tex->header.width, tex->header.height, tex->header.numpal, pngpal, 8,	// texture bytes are always packed so its always 4 bits
			 tex->data, tex->lumpRef, &pngsize);

	Wad_AddOutputLump(romWadFile.lump[tex->lumpRef].name, pngsize, png);
#else
	cache data;
	int size;
	char name[8];
	int pos = 0;
	const int palSize =
	    ((tex->header.numpal * NUMTEXPALETTES) * sizeof(dPalette_t));

	strncpy(name, romWadFile.lump[tex->lumpRef].name, 8);
	size = ((sizeof(d64ExTextureHeader_t) + tex->size) + palSize);

	data = (byte *) Mem_Alloc(size);

	memcpy(data, &tex->header, sizeof(d64ExTextureHeader_t));
	pos += sizeof(d64ExTextureHeader_t);

	memcpy((data + pos), tex->data, tex->size);
	pos += tex->size;

	memcpy(data + pos, tex->palette, palSize);

	Wad_AddOutputLump(name, size, data);

	Mem_Free((void **)&data);

#endif
}

//**************************************************************
//**************************************************************
//      Wad_AddOutputGfx
//
//      Sets up the GFX lump to be written to the output wad
//**************************************************************
//**************************************************************

void Wad_AddOutputGfx(gfxEx_t * gfx)
{
#ifdef USE_PNG
	png = Png_Create(gfx->width,
			 gfx->height,
			 16, gfx->palette, 8, gfx->data, gfx->lumpRef,
			 &pngsize);

	Wad_AddOutputLump(romWadFile.lump[gfx->lumpRef].name, pngsize, png);
#else
	cache data;
	int size;
	char name[8];
	int pos = 0;
	int i = 0;

	size = (8 + 768) + (gfx->width * gfx->height);
	strncpy(name, romWadFile.lump[gfx->lumpRef].name, 8);

	data = (byte *) Mem_Alloc(size);

	memcpy(data, gfx, 8);
	pos += 8;

	memcpy((data + pos), gfx->data, (gfx->width * gfx->height));
	pos += (gfx->width * gfx->height);

	for (i = 0; i < 256; i++) {
		memcpy((data + pos), &gfx->palette[i], 3);
		pos += 3;
	}

	Wad_AddOutputLump(name, size, data);

	Mem_Free((void **)&data);

#endif
}

//**************************************************************
//**************************************************************
//      Wad_AddOutputHudSprite
//**************************************************************
//**************************************************************

void Wad_AddOutputHudSprite(d64ExSpriteLump_t * sprite)
{
#ifdef USE_PNG
	png = Png_Create(sprite->sprite.width,
			 sprite->sprite.height,
			 16,
			 sprite->palette,
			 8, sprite->data, sprite->lumpRef, &pngsize);

	Wad_AddOutputLump(romWadFile.lump[sprite->lumpRef].name, pngsize, png);
#else
	cache data;
	int size;
	char name[8];
	int pos = 0;
	int i = 0;

	size = (8 + sprite->size) + 768;

	strncpy(name, romWadFile.lump[sprite->lumpRef].name, 8);

	data = (byte *) Mem_Alloc(size);
	memcpy(data, &sprite->sprite, 8);
	pos += 8;

	memcpy((data + pos), sprite->data, sprite->size);
	pos += sprite->size;

	for (i = 0; i < 256; i++) {
		memcpy((data + pos), &sprite->palette[i], 3);
		pos += 3;
	}

	Wad_AddOutputLump(name, size, data);

	Mem_Free((void **)&data);

#endif
}

//**************************************************************
//**************************************************************
//      Wad_AddOutputPalette
//
//      Adds the external palette lump to the output wad
//**************************************************************
//**************************************************************

void Wad_AddOutputPalette(d64PaletteLump_t * palette)
{
	cache data;
	int size;
	char name[8];
	int pos = 0;
	int i = 0;

	size = 768;
	strncpy(name, palette->name, 8);

	data = (byte *) Mem_Alloc(size);

	for (i = 0; i < 256; i++) {
		memcpy((data + pos), &palette->extPalLumps[i], 3);
		pos += 3;
	}

	Wad_AddOutputLump(name, size, data);
	Mem_Free((void **)&data);
}

//**************************************************************
//**************************************************************
//      Wad_AddOutputMidi
//
//      Adds a midi file to the output wad
//**************************************************************
//**************************************************************

void Wad_AddOutputMidi(midiheader_t * mthd, int index)
{
	cache data;
	int i;
	int ntracks;
	int pos = 0;

	data = (cache) Mem_Alloc(mthd->size);

#define COPYMIDIDATA(p, size)       \
    memcpy(data + pos, p, size);    \
    pos += size;                    \

	COPYMIDIDATA(mthd->header, 4);
	COPYMIDIDATA(&mthd->chunksize, 4);
	COPYMIDIDATA(&mthd->type, 2);
	COPYMIDIDATA(&mthd->ntracks, 2);
	COPYMIDIDATA(&mthd->delta, 2);

	ntracks = WGen_Swap16(mthd->ntracks);

	for (i = 0; i < ntracks; i++) {
		miditrack_t *track;
		int length;

		track = &mthd->tracks[i];
		length = WGen_Swap32(track->length);

		COPYMIDIDATA(track->header, 4);
		COPYMIDIDATA(&track->length, 4);
		COPYMIDIDATA(track->data, length);
	}

	Wad_AddOutputLump(sndlumpnames[index], mthd->size, data);

	Mem_Free((void **)&data);
}
