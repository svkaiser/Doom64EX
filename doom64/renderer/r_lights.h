// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
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

#ifndef D3DR_LIGHTS_H
#define D3DR_LIGHTS_H

#include "r_local.h"

enum {
    LIGHT_FLOOR,
    LIGHT_CEILING,
    LIGHT_THING,
    LIGHT_UPRWALL,
    LIGHT_LWRWALL
};

extern rcolor    bspColor[5];

rcolor R_GetSectorLight(byte alpha, word ptr);
void R_SetLightFactor(float lightfactor);
void R_RefreshBrightness(void);
void R_LightToVertex(vtx_t *v, int idx, word c);
void R_SetSegLineColor(seg_t *line, vtx_t* v, byte side);

#endif