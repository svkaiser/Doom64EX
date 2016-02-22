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

#ifndef G_CONTROLS_H
#define G_CONTROLS_H

#include "doomdef.h"

// villsa 01052014 - changed to 420
#define NUMKEYS         420

#define PCKF_DOUBLEUSE  0x4000
#define PCKF_UP         0x8000
#define PCKF_COUNTMASK  0x00ff

typedef enum {
    PCKEY_ATTACK,
    PCKEY_USE,
    PCKEY_STRAFE,
    PCKEY_FORWARD,
    PCKEY_BACK,
    PCKEY_LEFT,
    PCKEY_RIGHT,
    PCKEY_STRAFELEFT,
    PCKEY_STRAFERIGHT,
    PCKEY_RUN,
    PCKEY_JUMP,
    PCKEY_LOOKUP,
    PCKEY_LOOKDOWN,
    PCKEY_CENTER,
    NUM_PCKEYS
} pckeys_t;

typedef struct {
    int            mousex;
    int            mousey;
    int            joyx;
    int            joyy;
    int            key[NUM_PCKEYS];
    int            nextweapon;
    int            sdclicktime;
    int            fdclicktime;
    int            flags;
} playercontrols_t;

#define PCF_NEXTWEAPON    0x01
#define PCF_FDCLICK        0x02
#define PCF_FDCLICK2    0x04
#define PCF_SDCLICK        0x08
#define PCF_SDCLICK2    0x10
#define PCF_PREVWEAPON    0x20
#define PCF_GAMEPAD     0x40

extern playercontrols_t    Controls;
extern char *G_GetConfigFileName(void);

#endif
