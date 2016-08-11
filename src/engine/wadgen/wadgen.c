// Emacs style mode select       -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: wadgen.c 1230 2013-03-04 06:18:10Z svkaiser $
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
// $Revision: 1230 $
// $Date: 2013-03-03 22:18:10 -0800 (Sun, 03 Mar 2013) $
//
// DESCRIPTION: Global stuff and main application functions
//
//-----------------------------------------------------------------------------

#include <string.h>

#include "wadgen.h"
#include "rom.h"
#include "wad.h"
#include "level.h"
#include "sndfont.h"

#include <stdarg.h>
#include <i_system.h>

#define MAX_ARGS 256
char *ArgBuffer[MAX_ARGS + 1];
int myargc = 0;
char **myargv;

void WGen_ShutDownApplication(void);

//**************************************************************
//**************************************************************
//      WGen_AddDigest
//**************************************************************
//**************************************************************

static md5_context_t md5_context;

void WGen_AddDigest(char *name, int lump, int size)
{
	char buf[9];

	strncpy(buf, name, 8);
	buf[8] = '\0';

	MD5_UpdateString(&md5_context, buf);
	MD5_UpdateInt32(&md5_context, lump);
	MD5_UpdateInt32(&md5_context, size);
}

//**************************************************************
//**************************************************************
//      WGen_Process
//
//      The heart of the application. Opens the N64 rom and begins
//      extracting all the sprite and gfx data from it and begins
//      converting them into the ex format (Doom64 PC).
//
//      Content packages are extracted and added to the output wad.
//
//      The main IWAD is created afterwards.
//**************************************************************
//**************************************************************

void WGen_Process(char *path)
{
	int i = 0;
	char name[9];
	char *outFile;
	md5_digest_t digest;

	Rom_Open(path);

	Sound_Setup();
	Sprite_Setup();
	Gfx_Setup();
	Texture_Setup();
	Level_Setup();

	Wad_CreateOutput();

	MD5_Init(&md5_context);

	// Sprites
	Wad_AddOutputLump("S_START", 0, NULL);

	for (i = 0; i < spriteExCount; i++)
		Wad_AddOutputSprite(&exSpriteLump[i]);

	Wad_AddOutputLump("S_END", 0, NULL);

	// Sprite Palettes
	for (i = 0; i < extPalLumpCount; i++)
		Wad_AddOutputPalette(&d64PaletteLump[i]);

	// Textures
	Wad_AddOutputLump("T_START", 0, NULL);

	for (i = 0; d64ExTexture[i].data; i++)
		Wad_AddOutputTexture(&d64ExTexture[i]);

	Wad_AddOutputLump("T_END", 0, NULL);

	// Gfx
	Wad_AddOutputLump("G_START", 0, NULL);

	for (i = 0; gfxEx[i].data; i++)
		Wad_AddOutputGfx(&gfxEx[i]);

	// Hud Sprites
	for (i = spriteExCount; i < spriteExCount + hudSpriteExCount; i++)
		Wad_AddOutputHudSprite(&exSpriteLump[i]);

	Wad_AddOutputLump("G_END", 0, NULL);

#ifndef USE_SOUNDFONTS
	// Sounds
	Wad_AddOutputLump("DOOMSND", sn64size, (byte *) sn64);
	Wad_AddOutputLump("DOOMSEQ", sseqsize, (byte *) sseq);
#endif

	// Midi tracks
	Wad_AddOutputLump("DS_START", 0, NULL);

	for (i = 0; i < (int)sseq->nentry; i++)
		Wad_AddOutputMidi(&midis[i], i);

	Wad_AddOutputLump("DS_END", 0, NULL);

#ifndef USE_SOUNDFONTS
	// Sounds
	for (i = 0; i < sn64->nsfx; i++) {
		sprintf(name, "SFX_%03d", i);
		Wad_AddOutputLump(name, sfx[i].wavsize, wavtabledata[i]);
	}
#endif

	// Maps
	for (i = 0; i < MAXLEVELWADS; i++) {
		sprintf(name, "MAP%02d", i + 1);
		Wad_AddOutputLump(name, levelSize[i], levelData[i]);
	}

	MD5_Final(digest, &md5_context);

	Wad_AddOutputLump("CHECKSUM", sizeof(md5_digest_t), digest);

	// End of wad marker :)
	Wad_AddOutputLump("ENDOFWAD", 0, NULL);

	WGen_UpdateProgress("Writing IWAD File...");
	outFile = I_GetUserFile("doom64.wad");
	Wad_WriteOutput(outFile);

	// Done
	WGen_Printf("Successfully created %s", outFile);

	free(outFile);

	// Write out the soundfont file
#ifdef USE_SOUNDFONTS
	WGen_UpdateProgress("Writing Soundfont File...");
	SF_WriteSoundFont();
#endif
}

int M_CheckParm(char *);
void WGen_WadgenMain(void)
{
	char *p;
	char *start;
	char **arg;

	int parm = M_CheckParm("-wadgen");

	if (myargc < parm + 1) {
		WGen_Printf
		    ("Provide the location of the Doom64 Rom as a command line parameter.");
		exit(1);
	}
	WGen_Process(myargv[parm + 1]);
	WGen_ShutDownApplication();
}

//**************************************************************
//**************************************************************
//      WGen_Printf
//**************************************************************
//**************************************************************

void WGen_Printf(char *s, ...)
{
	char msg[1024];
	va_list v;

	va_start(v, s);
	vsprintf(msg, s, v);
	va_end(v);

#ifdef _WIN32
	MessageBox(NULL, msg, "Info", MB_OK | MB_ICONINFORMATION);
#else
	printf("%s\n", msg);
#endif
}

//**************************************************************
//**************************************************************
//      WGen_Complain
//**************************************************************
//**************************************************************

void WGen_Complain(char *fmt, ...)
{
	va_list va;
	char buff[1024];

	va_start(va, fmt);
	vsprintf(buff, fmt, va);
	va_end(va);
	WGen_Printf("ERROR: %s", buff);
	exit(1);
}

//**************************************************************
//**************************************************************
//      WGen_UpdateProgress
//**************************************************************
//**************************************************************

void WGen_UpdateProgress(char *fmt, ...)
{
	va_list va;
	char buff[1024];

	va_start(va, fmt);
	vsprintf(buff, fmt, va);
	va_end(va);

	WGen_Printf("%s\n", buff);
}

//**************************************************************
//**************************************************************
//      WGen_ShutDownApplication
//**************************************************************
//**************************************************************

void WGen_ShutDownApplication(void)
{
	Rom_Close();
}

//**************************************************************
//**************************************************************
//      WGen_ConvertN64Pal
//
//      N64 stores its RGB values a bit differently than that of the
//      PC/Windows format. This algorithm will convert any 16 bit
//      N64 palette (always 512 bytes) into a 768 byte PC/Win32 palette.
//
//      palette:        Palette struct to be used to convert the N64 pal to.
//      data:           Data that contains the pointer to the N64 pal data
//                              Its important to always use words when reading
//                              the N64 palette format for simplicity sakes.
//      indexes:        How many indexes are we going to convert? Usually
//                              16 or 256
//**************************************************************
//**************************************************************
#if 1
// new algorithm contributed by Daniel Swanson (DaniJ)
void WGen_ConvertN64Pal(dPalette_t * palette, word * data, int indexes)
{
	int i;
	short val;
	cache p = (cache) data;

	for (i = 0; i < indexes; i++) {
		// Read the next packed short from the input buffer.
		val = *(short *)p;
		p += 2;
		val = WGen_Swap16(val);

		// Unpack and expand to 8bpp, then flip from BGR to RGB.
		palette[i].b = (val & 0x003E) << 2;
		palette[i].g = (val & 0x07C0) >> 3;
		palette[i].r = (val & 0xF800) >> 8;
		palette[i].a = 0xff;	// Alpha is always 255..
	}
}
#else
// old algorithm
void WGen_ConvertN64Pal(dPalette_t * palette, word * data, int indexes)
{
	short g, r, b, start;
	int i, j;

	g = r = b = 0;

	for (i = 0; i < indexes; i++) {
		g = r = b = start = 0;
		for (j = 0; j < data[i]; j++) {
			if (j & 256) {
				g += 32;
				if (g >= 256) {
					r += 8;
					g = start;
				}
				if (r >= 256) {
					b += 8;
					r = 0;
				}
				if (b >= 256) {
					start += 8;
					b = 0;
				}
			}
		}
		if (r >= 32)
			r = r + (r / 32);
		if (g >= 32)
			g = g + (g / 32);
		if (b >= 32)
			b = b + (b / 32);

		palette[i].r = (byte) r;
		palette[i].g = (byte) g;
		palette[i].b = (byte) b;
		palette[i].a = 0xff;	// Alpha is always 255..
	}
}
#endif

//**************************************************************
//**************************************************************
//      WGen_Swap16
//
//      Needed mostly for sprites
//**************************************************************
//**************************************************************

int WGen_Swap16(int x)
{
	return (((word) (x & 0xff) << 8) | ((word) x >> 8));
}

//**************************************************************
//**************************************************************
//      WGen_Swap32
//**************************************************************
//**************************************************************

uint WGen_Swap32(unsigned int x)
{
	return (((x & 0xff) << 24) |
		(((x >> 8) & 0xff) << 16) |
		(((x >> 16) & 0xff) << 8) | ((x >> 24) & 0xff)
	    );
}
