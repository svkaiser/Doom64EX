#ifndef _WADGEN_H_
#define _WADGEN_H_

#ifdef _MSVC_VER
#include "SDL_config.h"
#else
#include <stdint.h>
#endif

#define PLATFORM_PC
//#define PLATFORM_DS

#ifdef _WIN32
#include <windows.h>
#include <commctrl.h>
#include <win32/resource.h>
#include <rpcdce.h>
#include <io.h>
#else
#ifdef __linux__
#include <linux/limits.h>
#else
#include <limits.h>
#endif
#define MAX_PATH PATH_MAX
#define ZeroMemory(a,l) memset(a, 0, l)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef __linux__
#define S_IREAD S_IRUSR
#define S_IWRITE S_IWUSR
#define S_IEXEC S_IXUSR
#endif

#ifdef PLATFORM_PC

#define USE_PNG
#include "png.h"

#endif

#define USE_SOUNDFONTS

#pragma warning(disable:4996)

typedef unsigned char byte;
typedef unsigned short word;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int bool;
typedef byte *cache;
typedef char path[MAX_PATH];

typedef struct {
	byte r;
	byte g;
	byte b;
	byte a;
} dPalette_t;

#ifdef _WIN32
extern HWND hwnd;
extern HWND hwndWait;
#endif
extern int myargc;
extern char **myargv;

int WGen_Swap16(int x);
uint WGen_Swap32(unsigned int x);

#define	_SWAP16(x)	WGen_Swap16(x)
#define _SWAP32(x)	WGen_Swap32(x)
#define _PAD4(x)	x += (4 - ((uint) x & 3)) & 3
#define _PAD8(x)	x += (8 - ((uint) x & 7)) & 7
#define _PAD16(x)	x += (16 - ((uint) x & 15)) & 15

void WGen_WadgenMain();
void WGen_Printf(char *s, ...);
void WGen_Complain(char *fmt, ...);
void WGen_UpdateProgress(char *fmt, ...);
void WGen_ConvertN64Pal(dPalette_t * palette, word * data, int indexes);
void WGen_AddDigest(char *name, int lump, int size);

#ifdef USE_PNG
cache Png_Create(int width, int height, int numpal, dPalette_t * pal,
		 int bits, cache data, int lump, int *size);
#endif

#define TOTALSTEPS	3500

#ifndef _WIN32
static inline char *strupr(char *in)
{
	unsigned char *ptr = (unsigned char *)in;
	while (*ptr != '\0') {
		int c = toupper(*ptr);
		*ptr++ = c;
	}
	return in;
}
#endif

#include "md5.h"

#endif
