// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2007-2012 Samuel Villarreal
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//    Main program
//
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#include "i_opndir.h"
#else
#include <dirent.h>
#endif

#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "SDL.h"
#include "i_video.h"
#include "m_misc.h"
#include "i_system.h"
#include "con_console.h"

void WGen_WadgenMain();

const char version_date[] = __DATE__;

//
// _dprintf
//

void _dprintf(const char *s, ...) {
    static char msg[MAX_MESSAGE_SIZE];
    va_list    va;

    va_start(va, s);
    vsprintf(msg, s, va);
    va_end(va);

    players[consoleplayer].message = msg;
}

//
// dmemcpy
//

void *dmemcpy(void *s1, const void *s2, size_t n) {
    char *r1 = (char*) s1;
    const char *r2 = (char*) s2;

    while(n) {
        *r1++ = *r2++;
        --n;
    }

    return s1;
}

//
// dmemset
//

void *dmemset(void *s, dword c, size_t n) {
    char *p = (char *)s;

    while(n) {
        *p++ = (char)c;
        --n;
    }

    return s;
}

//
// dstrcpy
//

char *dstrcpy(char *dest, const char *src) {
    dstrncpy(dest, src, dstrlen(src));
    return dest;
}

//
// dstrncpy
//

void dstrncpy(char *dest, const char *src, int maxcount) {
    char *p1 = dest;
    const char *p2 = src;
    while((maxcount--) >= 0) {
        *p1++ = *p2++;
    }
}

//
// dstrcmp
//

int dstrcmp(const char *s1, const char *s2) {
    while(*s1 && *s2) {
        if(*s1 != *s2) {
            return *s2 - *s1;
        }
        s1++;
        s2++;
    }
    if(*s1 != *s2) {
        return *s2 - *s1;
    }
    return 0;
}

//
// dstrncmp
//

int dstrncmp(const char *s1, const char *s2, int len) {
    while(*s1 && *s2) {
        if(*s1 != *s2) {
            return *s2 - *s1;
        }
        s1++;
        s2++;
        if(!--len) {
            return 0;
        }
    }
    if(*s1 != *s2) {
        return *s2 - *s1;
    }
    return 0;
}

//
// dstricmp
//

int dstricmp(const char *s1, const char *s2) {
    return strcasecmp(s1, s2);
}

//
// dstrnicmp
//

int dstrnicmp(const char *s1, const char *s2, int len) {
    return strncasecmp(s1, s2, len);
}

//
// dstrupr
//

void dstrupr(char *s) {
    char c;

    while((c = *s) != 0) {
        if(c >= 'a' && c <= 'z') {
            c -= 'a'-'A';
        }
        *s++ = c;
    }
}

//
// dstrlwr
//

void dstrlwr(char *s) {
    char c;

    while((c = *s) != 0) {
        if(c >= 'A' && c <= 'Z') {
            c += 32;
        }
        *s++ = c;
    }
}

//
// dstrnlen
//

int dstrlen(const char *string) {
    int rc = 0;
    if(string)
        while(*(string++)) {
            rc++;
        }
    else {
        rc = -1;
    }

    return rc;
}

//
// dstrrchr
//

char *dstrrchr(char *s, char c) {
    int len = dstrlen(s);
    s += len;
    while(len--)
        if(*--s == c) {
            return s;
        }
    return 0;
}

//
// dstrcat
//

void dstrcat(char *dest, const char *src) {
    dest += dstrlen(dest);
    dstrcpy(dest, src);
}

//
// dstrstr
//

char *dstrstr(char *s1, char *s2) {
    char *p = s1;
    int len = dstrlen(s2);

    for(; (p = dstrrchr(p, *s2)) != 0; p++) {
        if(dstrncmp(p, s2, len) == 0) {
            return p;
        }
    }

    return 0;
}

//
// datoi
//

int datoi(const char *str) {
    int val;
    int sign;
    int c;

    if(*str == '-') {
        sign = -1;
        str++;
    }
    else {
        sign = 1;
    }

    val = 0;

    // check for hex

    if(str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
        while(1) {
            c = *str++;
            if(c >= '0' && c <= '9') {
                val = (val<<4) + c - '0';
            }
            else if(c >= 'a' && c <= 'f') {
                val = (val<<4) + c - 'a' + 10;
            }
            else if(c >= 'A' && c <= 'F') {
                val = (val<<4) + c - 'A' + 10;
            }
            else {
                return val*sign;
            }
        }
    }

    // check for character

    if(str[0] == '\'') {
        return sign * str[1];
    }

    // assume decimal

    while(1) {
        c = *str++;
        if(c <'0' || c > '9') {
            return val*sign;
        }

        val = val*10 + c - '0';
    }

    return 0;
}

//
// datof
//

float datof(char *str) {
    double    val;
    int        sign;
    int        c;
    int        decimal, total;

    if(*str == '-') {
        sign = -1;
        str++;
    }
    else {
        sign = 1;
    }

    val = 0;

    // check for hex
    if(str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
        while(1) {
            c = *str++;
            if(c >= '0' && c <= '9') {
                val = (val*16) + c - '0';
            }
            else if(c >= 'a' && c <= 'f') {
                val = (val*16) + c - 'a' + 10;
            }
            else if(c >= 'A' && c <= 'F') {
                val = (val*16) + c - 'A' + 10;
            }
            else {
                return (float)val*sign;
            }
        }
    }

    // check for character
    if(str[0] == '\'') {
        return (float)sign * str[1];
    }

    // assume decimal
    decimal = -1;
    total = 0;
    while(1) {
        c = *str++;
        if(c == '.') {
            decimal = total;
            continue;
        }
        if(c <'0' || c > '9') {
            break;
        }
        val = val*10 + c - '0';
        total++;
    }

    if(decimal == -1) {
        return (float)val*sign;
    }

    while(total > decimal) {
        val /= 10;
        total--;
    }

    return (float)val*sign;
}

//
// dhtoi
//

int dhtoi(char* str) {
    char *s;
    int num;

    num = 0;
    s = str;

    while(*s) {
        num <<= 4;
        if(*s >= '0' && *s <= '9') {
            num += *s-'0';
        }
        else if(*s >= 'a' && *s <= 'f') {
            num += 10 + *s-'a';
        }
        else if(*s >= 'A' && *s <= 'F') {
            num += 10 + *s-'A';
        }
        else {
            return 0;
        }
        s++;
    }

    return num;
}

//
// dfcmp
//

dboolean dfcmp(float f1, float f2) {
    float precision = 0.00001f;
    if(((f1 - precision) < f2) &&
            ((f1 + precision) > f2)) {
        return true;
    }
    else {
        return false;
    }
}

//
// D_abs
//

#ifdef _MSC_VER
#pragma warning( disable : 4035 )
int D_abs(int x) {
    __asm {
        mov eax,x
        cdq
        xor eax,edx
        sub eax,edx
    }
}
#else
int D_abs(int x) {
    fixed_t _t = (x),_s;
    _s = _t >> (8*sizeof _t-1);
    return (_t^_s)-_s;
}
#endif

//
// dfabs
//

float D_fabs(float x) {
    return x < 0 ? -x : x;
}

//
// dsprintf
//

int dsprintf(char *buf, const char *format, ...) {
    va_list arg;
    int x;

    va_start(arg, format);
#ifdef HAVE_VSNPRINTF
    x = vsnprintf(buf, dstrlen(buf), format, arg);
#else
    x = vsprintf(buf, format, arg);
#endif
    va_end(arg);

    return x;
}

//
// dsnprintf
//

int dsnprintf(char *src, size_t n, const char *str, ...) {
    int x;
    va_list argptr;
    va_start(argptr, str);

#ifdef HAVE_VSNPRINTF
    x = vsnprintf(src, n, str, argptr);
#else
    x = vsprintf(src, str, argptr);
#endif

    va_end(argptr);

    return x;
}
