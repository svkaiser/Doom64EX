#ifndef _WADGEN_TEX_H_
#define _WADGEN_TEX_H_

#define MAXPALETTES		16
#define NUMTEXPALETTES	16
#define MAXTEXTURES		800

// N64 Texture format

typedef struct {
	word id;
	word numpal;
	word wshift;
	word hshift;
} d64RawTextureHeader_t;

typedef struct {
	d64RawTextureHeader_t header;
	cache data;
	word *palette;
} d64RawTexture_t;

// D64EX texture format

typedef struct {
	word width;
	word height;
	word numpal;
	word compressed;
} d64ExTextureHeader_t;

typedef struct {
	d64ExTextureHeader_t header;
	cache data;
	dPalette_t palette[MAXPALETTES][NUMTEXPALETTES];
	int size;
	int lumpRef;
} d64ExTexture_t;

extern d64ExTexture_t d64ExTexture[MAXTEXTURES];

void Texture_Setup(void);

#endif
