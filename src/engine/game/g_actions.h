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

#ifndef G_ACTIONS_H
#define G_ACTIONS_H

#define MAX_ACTIONPARAM        2

typedef void (*actionproc_t)(int64 data, char **param);

#define CMD(name) void CMD_ ## name(int64 data, char** param)

void        G_InitActions(void);
dboolean    G_ActionResponder(event_t *ev);
void        G_AddCommand(const char *name, actionproc_t proc, int64 data);
void        G_ActionTicker(void);
void        G_ExecuteCommand(char *action);
void        G_BindActionByName(char *key, char *action);
dboolean    G_BindActionByEvent(event_t *ev, char *action);
void        G_ShowBinding(char *key);
void        G_GetActionBindings(char *buff, const char *action);
void        G_UnbindAction(const char *action);
int         G_ListCommands(void);
void        G_OutputBindings(FILE *fh);
void        G_DoCmdMouseMove(int x, int y);
void        G_DoCmdGamepadMove(event_t *ev);

extern dboolean    ButtonAction;

#endif
