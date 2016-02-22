#ifndef _WADGEN_SPRITE_H_
#define _WADGEN_SPRITE_H_

#define MAX_SPRITES	2048
#define CMPPALCOUNT	16

extern int spriteExCount;
extern int hudSpriteExCount;
extern int extPalLumpCount;

#define INSPRITELIST(x) (x >= Wad_GetLumpNum("S_START") && x <= Wad_GetLumpNum("S_END"))

//N64 sprite format
typedef struct {
	short tiles;		// how many tiles the sprite is divided into
	short compressed;	// >=0 = 'two for one' byte compression, -1 = not compressed
	short cmpsize;		// actual compressed size (0 if not compressed)
	short xoffs;		// draw x offset
	short yoffs;		// draw y offset
	short width;		// draw width
	short height;		// draw height
	short tileheight;	// y height per tile piece
} d64RawSprite_t;

//D64EX sprite format
typedef struct {
	word width;		// draw width
	word height;		// draw height
	short offsetx;		// draw x offset
	short offsety;		// draw y offset
	short useExtPal;	// true = use internal palette, false = use external lump palette (for hi colored sprites)
} d64ExSprite_t;

typedef struct {
	d64ExSprite_t sprite;
	cache data;
	dPalette_t palette[256];
	int size;
	int lumpRef;
} d64ExSpriteLump_t;

extern d64ExSpriteLump_t exSpriteLump[MAX_SPRITES];

typedef struct {
	dPalette_t extPalLumps[256];
	char name[8];
} d64PaletteLump_t;

extern d64PaletteLump_t d64PaletteLump[24];

void Sprite_Setup(void);

#endif
