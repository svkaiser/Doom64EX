// -*- mode: c++ -*-
#ifndef __DOOM64EX_MAP__59976677
#define __DOOM64EX_MAP__59976677

#include <imp/Wad>

void* W_GetMapLump(int lump);

void W_CacheMapLump(int map);

void W_FreeMapLump();

int W_MapLumpLength(int lump);

void P_InitTextureHashTable(void);

uint32 P_GetTextureHashKey(int hash);

#endif //__DOOM64EX_MAP__59976677
