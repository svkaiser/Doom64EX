// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2003 Tim Stump
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

#ifndef R_CLIPPER_H
#define R_CLIPPER_H

dboolean    R_Clipper_SafeCheckRange(angle_t startAngle, angle_t endAngle);
void        R_Clipper_SafeAddClipRange(angle_t startangle, angle_t endangle);
void        R_Clipper_Clear(void);

extern float frustum[6][4];

angle_t     R_FrustumAngle(void);
void        R_FrustrumSetup(void);
dboolean    R_FrustrumTestVertex(vtx_t* vertex, int count);

#endif