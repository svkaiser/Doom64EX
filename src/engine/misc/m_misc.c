// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 1997 Midway Home Entertainment, Inc
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
//      Main loop menu stuff.
//      Default Config File.
//        Executable arguments
//        BBox stuff
//
//-----------------------------------------------------------------------------

#ifdef _WIN32
#include <io.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
//#include <GL/glu.h>

#include "doomstat.h"
#include "m_misc.h"
#include "z_zone.h"
#include "g_local.h"
#include "st_stuff.h"
#include "i_png.h"
#include "gl_texture.h"
#include "p_saveg.h"

int        myargc;
char**    myargv;


//
// M_CheckParm
// Checks for the given parameter
// in the program's command line arguments.
// Returns the argument number (1 to argc-1)
// or 0 if not present

int M_CheckParm(char *check) {
    int        i;

    for(i = 1; i<myargc; i++) {
        if(!dstricmp(check, myargv[i])) { //strcasecmp
            return i;
        }
    }

    return 0;
}

//
// M_ClearBox
//

void M_ClearBox(fixed_t *box) {
    box[BOXTOP] = box[BOXRIGHT] = D_MININT;
    box[BOXBOTTOM] = box[BOXLEFT] = D_MAXINT;
}

//
// M_AddToBox
//

void M_AddToBox(fixed_t* box, fixed_t x, fixed_t y) {
    if(x<box[BOXLEFT]) {
        box[BOXLEFT] = x;
    }
    else if(x>box[BOXRIGHT]) {
        box[BOXRIGHT] = x;
    }
    if(y<box[BOXBOTTOM]) {
        box[BOXBOTTOM] = y;
    }
    else if(y>box[BOXTOP]) {
        box[BOXTOP] = y;
    }
}


//
// M_WriteFile
//

dboolean M_WriteFile(char const* name, void* source, int length) {
    FILE *fp;
    dboolean result;

    errno = 0;

    if(!(fp = fopen(name, "wb"))) {
        return 0;
    }

    I_BeginRead();
    result = (fwrite(source, 1, length, fp) == (dword)length);
    fclose(fp);

    if(!result) {
        remove(name);
    }

    return result;
}

//
// M_WriteTextFile
//

dboolean M_WriteTextFile(char const* name, char* source, int length) {
    int handle;
    int count;

    handle = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0666);

    if(handle == -1) {
        return false;
    }

    count = write(handle, source, length);
    close(handle);

    if(count < length) {
        return false;
    }

    return true;
}


//
// M_ReadFile
//

int M_ReadFile(char const* name, byte** buffer) {
    FILE *fp;

    errno = 0;

    if((fp = fopen(name, "rb"))) {
        size_t length;

        I_BeginRead();

        fseek(fp, 0, SEEK_END);
        length = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        *buffer = Z_Malloc(length, PU_STATIC, 0);

        if(fread(*buffer, 1, length, fp) == length) {
            fclose(fp);
            return length;
        }

        fclose(fp);
    }

    //I_Error("M_ReadFile: Couldn't read file %s: %s", name,
    //errno ? strerror(errno) : "(Unknown Error)");

    return -1;
}

//
// M_FileLength
//

long M_FileLength(FILE *handle) {
    long savedpos;
    long length;

    // save the current position in the file
    savedpos = ftell(handle);

    // jump to the end and find the length
    fseek(handle, 0, SEEK_END);
    length = ftell(handle);

    // go back to the old location
    fseek(handle, savedpos, SEEK_SET);

    return length;
}

//
// M_NormalizeSlashes
//
// Remove trailing slashes, translate backslashes to slashes
// The string to normalize is passed and returned in str
//
// killough 11/98: rewritten
//

void M_NormalizeSlashes(char *str) {
    char *p;

    // Convert all slashes/backslashes to DIR_SEPARATOR
    for(p = str; *p; p++) {
        if((*p == '/' || *p == '\\') && *p != DIR_SEPARATOR) {
            *p = DIR_SEPARATOR;
        }
    }

    // Collapse multiple slashes
    for(p = str; (*str++ = *p);)
        if(*p++ == DIR_SEPARATOR)
            while(*p == DIR_SEPARATOR) {
                p++;
            }
}

//
// W_FileExists
// Check if a wad file exists
//

int M_FileExists(char *filename) {
    FILE *fstream;

    fstream = fopen(filename, "r");

    if(fstream != NULL) {
        fclose(fstream);
        return 1;
    }
    else {
        // If we can't open because the file is a directory, the
        // "file" exists at least!

        if(errno == 21) {
            return 2;
        }
    }

    return 0;
}

//
// M_SaveDefaults
//

void M_SaveDefaults(void) {
    FILE        *fh;

    fh=fopen(G_GetConfigFileName(), "wt");
    if(fh) {
        G_OutputBindings(fh);
        fclose(fh);
    }
}

//
// M_LoadDefaults
//

void M_LoadDefaults(void) {
    G_LoadSettings();
}

//
// M_ScreenShot
//

void M_ScreenShot(void) {
    char    name[13];
    int     shotnum=0;
    byte    *buff;
    Image   *image;
    int     size;

    while(shotnum < 1000) {
        sprintf(name, "sshot%03d.png", shotnum);
        if(access(name, 0) != 0) {
            break;
        }
        shotnum++;
    }

    if(shotnum >= 1000) {
        return;
    }

    if((video_height % 2)) {  // height must be power of 2
        return;
    }

    buff = GL_GetScreenBuffer(0, 0, video_width, video_height);
    size = 0;

    // Get PNG image

    image = Image_New_FromData(PF_RGB, video_width, video_height, buff);
    Image_Save(image, name, "png");
    Image_Free(image);

    Z_Free(buff);

    I_Printf("Saved Screenshot %s\n", name);
}

//
// M_CacheThumbNail
// Thumbnails are assumed they are
// uncompressed 128x128 RGB textures
//

int M_CacheThumbNail(byte** data) {
    byte* buff;
    byte* tbn;
    Image* image;

    buff = GL_GetScreenBuffer(0, 0, video_width, video_height);
    tbn = Z_Calloc(SAVEGAMETBSIZE, PU_STATIC, 0);

    image = Image_New_FromData(PF_RGB, video_width, video_height, buff);
    Image_Scale(image, 128, 128);
    dmemcpy(tbn, Image_GetData(image), 128 * 128 * 3);
    Image_Free(image);

    Z_Free(buff);

    *data = tbn;
    return SAVEGAMETBSIZE;
}


