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

#ifndef D3DR_MAIN_H
#define D3DR_MAIN_H

#include "t_bsp.h"
#include "d_player.h"
#include "w_wad.h"
#include "gl_main.h"
#include "con_cvar.h"

extern fixed_t      viewx;
extern fixed_t      viewy;
extern fixed_t      viewz;
extern float        fviewx;
extern float        fviewy;
extern float        fviewz;
extern angle_t      viewangle;
extern angle_t      viewpitch;
extern fixed_t      quakeviewx;
extern fixed_t      quakeviewy;
extern angle_t      viewangleoffset;
extern rcolor       flashcolor;

extern float        viewsin[2];
extern float        viewcos[2];
extern float        viewoffset;
extern int          skytexture;
extern player_t     *renderplayer;
extern int          logoAlpha;
extern fixed_t      scrollfrac;
extern int          vertCount;

extern unsigned int renderTic;
extern unsigned int spriteRenderTic;
extern unsigned int glBindCalls;

extern dboolean     bRenderSky;

CVAR_EXTERNAL(r_fov);
CVAR_EXTERNAL(r_fillmode);
CVAR_EXTERNAL(r_uniformtime);
CVAR_EXTERNAL(r_drawtrace);

void R_Init(void);
void R_RenderPlayerView(player_t *player);
subsector_t *R_PointInSubsector(fixed_t x, fixed_t y);
angle_t R_PointToAngle2(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2);
angle_t R_PointToAngle(fixed_t x, fixed_t y);//note difference from sw version
angle_t R_PointToPitch(fixed_t z1, fixed_t z2, fixed_t dist);
void R_PrecacheLevel(void);
int R_PointOnSide(fixed_t x, fixed_t y, node_t *node);
fixed_t R_Interpolate(fixed_t ticframe, fixed_t updateframe, dboolean enable);
void R_SetupLevel(void);
void R_SetViewAngleOffset(angle_t angle);
void R_SetViewOffset(int offset);
void R_DrawWireframe(dboolean enable);    //villsa
void R_RegisterCvars(void);
void R_SetViewMatrix(void);
void R_RenderWorld(void);
void R_RenderBSPNode(int bspnum);
void R_AllocSubsectorBuffer(void);

#endif