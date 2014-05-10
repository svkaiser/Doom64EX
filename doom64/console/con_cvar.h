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

#ifndef CON_CVAR_H
#define CON_CVAR_H

#include "doomtype.h"

typedef struct cvar_s {
    char*           name;
    char*           string;
    dboolean        nonclient;
    void (*callback)(void*);
    float           value;
    char*           defvalue;
    struct cvar_s*  next;
} cvar_t;

#define CVAR(name, str)                                     \
    cvar_t name = { # name, # str, 0, NULL }

#define CVAR_CMD(name, str)                                 \
    void CvarCmd_ ## name(cvar_t* cvar);                    \
    cvar_t name = { # name, # str, 0, CvarCmd_ ## name };   \
    void CvarCmd_ ## name(cvar_t* cvar)

#define CVAR_PARAM(name, str, var, flags)                   \
    void CvarCmd_ ## name(cvar_t* cvar);                    \
    cvar_t name = { # name, #str, 0, CvarCmd_ ## name };    \
    void CvarCmd_ ## name(cvar_t* cvar)                     \
    {                                                       \
        if(cvar->value > 0)                                 \
            var |= flags;                                   \
        else                                                \
            var &= ~flags;                                  \
    }

#define NETCVAR(name, str)                                  \
    cvar_t name = { # name, # str, 1, NULL }

#define NETCVAR_CMD(name, str)                              \
    void CvarCmd_ ## name(cvar_t* cvar);                    \
    cvar_t name = { # name, # str, 1, CvarCmd_ ## name };   \
    void CvarCmd_ ## name(cvar_t* cvar)

#define NETCVAR_PARAM(name, str, var, flags)                \
    void CvarCmd_ ## name(cvar_t* cvar);                    \
    cvar_t name = { # name, #str, 0, CvarCmd_ ## name };    \
    void CvarCmd_ ## name(cvar_t* cvar)                     \
    {                                                       \
        if(cvar->value > 0)                                 \
            var |= flags;                                   \
        else                                                \
            var &= ~flags;                                  \
    }

#define CVAR_EXTERNAL(name)                                 \
    extern cvar_t name

extern cvar_t*  cvarcap;

void CON_CvarInit(void);
void CON_CvarRegister(cvar_t *variable);
void CON_CvarSet(char *var_name, char *value);
void CON_CvarSetValue(char *var_name, float value);
void CON_CvarAutoComplete(char *partial);
cvar_t *CON_CvarGet(char *name);

#endif

