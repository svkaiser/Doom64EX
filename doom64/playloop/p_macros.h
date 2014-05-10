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

#ifndef __P_MACROS__
#define __P_MACROS__


#ifdef __GNUG__
#pragma interface
#endif

#define MAXQUEUELIST    16

extern int taglist[MAXQUEUELIST];
extern int taglistidx;

void P_QueueSpecial(mobj_t* mobj);

extern thinker_t    *macrothinker;
extern macrodef_t   *macro;
extern macrodata_t  *nextmacro;
extern mobj_t       *mobjmacro;
extern short        macrocounter;
extern short        macroid;

void P_InitMacroVars(void);
void P_ToggleMacros(int tag, dboolean toggleon);
void P_MacroDetachThinker(thinker_t *thinker);
void P_RunMacros(void);

int P_StartMacro(mobj_t *thing, line_t *line);
int P_SuspendMacro(int tag);
int P_InitMacroCounter(int counts);

#endif