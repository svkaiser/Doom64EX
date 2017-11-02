#ifndef _WADGEN_GFX_H_
#define _WADGEN_GFX_H_

#define MAXGFXEXITEMS	128

typedef struct {
	word compressed;	//-1 = not compressed (all image lumps are not compressed anyways)
	word u2;		//unknown
	word width;		//width (short swapped)
	word height;		//height (short swapped)
} gfxRomHeader_t;

typedef struct {
	gfxRomHeader_t header;
	cache data;
	word palette[256];	//n64 format (16 bit rgb)
} gfxRom_t;

typedef struct {
	word width;
	word height;
	word size;		//no longer used
	word palPos;		//no longer used
	cache data;
	dPalette_t palette[256];
	int lumpRef;
} gfxEx_t;

extern gfxEx_t gfxEx[MAXGFXEXITEMS];

void Gfx_Setup(void);

#endif
