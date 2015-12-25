// Emacs style mode select       -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: Png.c 922 2011-08-13 00:49:06Z svkaiser $
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
// $Revision: 922 $
// $Date: 2011-08-12 17:49:06 -0700 (Fri, 12 Aug 2011) $
//
// DESCRIPTION: PNG image format setup
//
//-----------------------------------------------------------------------------
#ifdef RCSID
static const char rcsid[] = "$Id: Png.c 922 2011-08-13 00:49:06Z svkaiser $";
#endif

#include "wadgen.h"
#include "wad.h"
#include "sprite.h"

#ifdef USE_PNG

static cache writeData;
static size_t current = 0;

//**************************************************************
//**************************************************************
//      Png_WriteData
//
//      Work with data writing through memory
//**************************************************************
//**************************************************************

static void Png_WriteData(png_structp png_ptr, cache data, size_t length)
{
	writeData = (byte *) realloc(writeData, current + length);
	memcpy(writeData + current, data, length);
	current += length;
}

//**************************************************************
//**************************************************************
//      Png_Create
//
//      Create a PNG image through memory
//**************************************************************
//**************************************************************

cache
Png_Create(int width, int height, int numpal, dPalette_t * pal,
	   int bits, cache data, int lump, int *size)
{
	int i = 0;
	int x = 0;
	int j = 0;
	cache image;
	cache out;
	cache *row_pointers;
	png_structp png_ptr;
	png_infop info_ptr;
	png_colorp palette;

	// setup png pointer
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (png_ptr == NULL) {
		WGen_Complain("Png_Create: Failed getting png_ptr");
		return NULL;
	}
	// setup info pointer
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		png_destroy_write_struct(&png_ptr, NULL);
		WGen_Complain("Png_Create: Failed getting info_ptr");
		return NULL;
	}
	// what does this do again?
	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		WGen_Complain("Png_Create: Failed on setjmp");
		return NULL;
	}
	// setup custom data writing procedure
	png_set_write_fn(png_ptr, NULL, Png_WriteData, NULL);

	// setup image
	png_set_IHDR(png_ptr,
		     info_ptr,
		     width,
		     height,
		     bits,
		     PNG_COLOR_TYPE_PALETTE,
		     PNG_INTERLACE_NONE,
		     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_DEFAULT);

	// setup palette
	palette = (png_colorp) Mem_Alloc((16 * numpal) * sizeof(png_color));

	// copy dPalette_t data over to png_colorp
	for (x = 0, j = 0; x < numpal; x++) {
		for (i = 0; i < 16; i++) {
			palette[j].red = pal[j].r;
			palette[j].green = pal[j].g;
			palette[j].blue = pal[j].b;
			j++;
		}
	}

	i = 0;

	// add palette to png
	png_set_PLTE(png_ptr, info_ptr, palette, (16 * numpal));

	// set transparent index
	if (palette[0].red == 0 && palette[0].green == 0
	    && palette[0].blue == 0) {
		char tmp[9];

		strncpy(tmp, romWadFile.lump[lump].name, 8);
		tmp[0] -= (char)0x80;
		tmp[8] = 0;

		// Exempt these lumps
		if (strcmp(tmp, "FIRE") &&	/*strcmp(tmp, "USLEGAL") &&
						   strcmp(tmp, "TITLE") && */ strcmp(tmp, "EVIL") &&
		    /*strcmp(tmp, "IDCRED1") && strcmp(tmp, "WMSCRED1") && */
		    strcmp(tmp, "SPACE") && strcmp(tmp, "CLOUD") &&
		    strcmp(tmp, "FINAL"))
			png_set_tRNS(png_ptr, info_ptr, (png_bytep) & i, 1,
				     NULL);
	}
	// add png info to data
	png_write_info(png_ptr, info_ptr);

	// add offset chunk if png is a sprite
	if (INSPRITELIST(lump)) {
		for (i = 0; i < spriteExCount; i++) {
			if (exSpriteLump[i].lumpRef == lump) {
				int offs[2];

				offs[0] =
				    WGen_Swap32(exSpriteLump[i].sprite.offsetx);
				offs[1] =
				    WGen_Swap32(exSpriteLump[i].sprite.offsety);

				png_write_chunk(png_ptr, "grAb", (byte *) offs,
						8);
				break;
			}
		}
	}
	// setup packing if needed
	png_set_packing(png_ptr);
	png_set_packswap(png_ptr);

	// copy data over
	image = data;
	row_pointers = (cache *) Mem_Alloc(sizeof(byte *) * height);

	for (i = 0; i < height; i++) {
		row_pointers[i] = (cache) Mem_Alloc(width);
		if (bits == 4) {
			for (j = 0; j < width; j += 2) {
				row_pointers[i][j] = *image & 0xf;
				row_pointers[i][j + 1] = *image >> 4;
				image++;
			}
		} else {
			for (j = 0; j < width; j++) {
				row_pointers[i][j] = *image;
				image++;
			}
		}

		png_write_rows(png_ptr, &row_pointers[i], 1);
	}

	// cleanup
	png_write_end(png_ptr, info_ptr);
	Mem_Free((void **)&palette);
	Mem_Free((void **)row_pointers);
	palette = NULL;
	row_pointers = NULL;
	png_destroy_write_struct(&png_ptr, &info_ptr);

	// allocate output
	out = (cache) Mem_Alloc(current);
	memcpy(out, writeData, current);
	*size = current;

	free(writeData);
	writeData = NULL;
	current = 0;

	return out;
}

#endif				// USE_PNG
