// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1999-2000 Paul Brook
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

#ifndef CON_CONSOLE_H
#define CON_CONSOLE_H

#include "d_event.h"
#include "gl_main.h"
#include "con_cvar.h"

#define MAX_CONSOLE_INPUT_LEN    80
extern char     console_inputbuffer[];
extern int      console_inputlength;
extern dboolean console_initialized;

#define CONCLEARINPUT() (dmemset(console_inputbuffer+1, 0, MAX_CONSOLE_INPUT_LEN-1))

void CON_Init(void);
void CON_AddText(char *text);
void CON_Printf(rcolor clr, const char *s, ...);
void CON_Warnf(const char *s, ...);
void CON_DPrintf(const char *s, ...);
void CON_Draw(void);
void CON_AddLine(char *line, int len);
void CON_Ticker(void);

dboolean CON_Responder(event_t* ev);

#endif