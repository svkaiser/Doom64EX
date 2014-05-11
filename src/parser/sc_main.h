// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
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


#ifndef __SC_MAIN__
#define __SC_MAIN__

typedef struct {
    char    token[512];
    char*   buffer;
    char*   pointer_start;
    char*   pointer_end;
    int     linepos;
    int     rowpos;
    int     buffpos;
    int     buffsize;
    void (*open)(void*);
    void (*close)(void);
    void (*compare)(void*);
    int (*find)(dboolean);
    char(*fgetchar)(void);
    void (*rewind)(void);
    char*   (*getstring)(void);
    int (*getint)(void);
    int (*setdata)(void*, void*);
    int (*readtokens)(void);
    void (*error)(void*);
} scparser_t;

extern scparser_t sc_parser;

typedef struct {
    char*   token;
    int64   ptroffset;
    char    type;
} scdatatable_t;

void SC_Init(void);

#endif // __SC_MAIN__
