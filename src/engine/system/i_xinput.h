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

#ifndef __I_XINPUT__
#define __I_XINPUT__

#ifdef _WIN32
#define _USE_XINPUT
#endif


#ifdef _USE_XINPUT

#include <windows.h>

//
// use Microsoft's bullcrap annotation macros if not already defined
//
#if !defined(__ATTR_SAL)
#include "Ext/sal.h"
#endif

#include <XInput.h>

#define XINPUT_BUTTONS  14
#define XINPUT_MAX_STICK_THRESHOLD  32768

//
// Macro definitions for trigger buttons and thumb sticks
//

#define XINPUT_GAMEPAD_LEFT_TRIGGER     0x10000
#define XINPUT_GAMEPAD_RIGHT_TRIGGER    0x20000
#define XINPUT_GAMEPAD_LEFT_STICK       0x40000
#define XINPUT_GAMEPAD_RIGHT_STICK      0x80000

#define BUTTON_DPAD_UP                  400
#define BUTTON_DPAD_DOWN                401
#define BUTTON_DPAD_LEFT                402
#define BUTTON_DPAD_RIGHT               403
#define BUTTON_START                    404
#define BUTTON_BACK                     405
#define BUTTON_LEFT_THUMB               406
#define BUTTON_RIGHT_THUMB              407
#define BUTTON_LEFT_SHOULDER            408
#define BUTTON_RIGHT_SHOULDER           409
#define BUTTON_A                        410
#define BUTTON_B                        411
#define BUTTON_X                        412
#define BUTTON_Y                        413
#define BUTTON_LEFT_TRIGGER             414
#define BUTTON_RIGHT_TRIGGER            415

//
// Structures used for XInput
//

//
// buttons structure
// contains input information for buttons, left/right triggers
// and analog stick movement
//
typedef struct {
    word    data;
    byte    ltrigger;
    byte    rtrigger;
    short   lx;
    short   ly;
    short   rx;
    short   ry;
} xinputbuttons_t;

//
// state structure must be structured out
// like this in order to properly fetch
// state data
//
typedef struct {
    dword           id;
    xinputbuttons_t buttons;
} xinputstate_t;

//
// rumble structure
//
typedef struct {
    word lMotorSpeed;
    word rMotorSpeed;
} xinputrumble_t;

//
// kex gamepad structure for xinput
//
typedef struct {
    xinputstate_t   state;                      // gamepad state
    xinputrumble_t  vibration;                  // rumble data
    dboolean        connected;                  // is controller connected?
    dboolean        available;                  // is api available? can be disabled with -noxinput
    int             oldbuttons;                 // button inputs from previous tic
    int             refiretic[XINPUT_BUTTONS];  // how long to refire held down buttons?
    float           rxthreshold;                // right stick x-axis threshold
    float           rythreshold;                // right stick y-axis threshold
    word            lMotorWindDown;             // left motor wind down speed
    word            rMotorWindDown;             // right motor wind down speed
} xinputgamepad_t;

extern xinputgamepad_t xgamepad;
extern int xbtnkeys[XINPUT_BUTTONS + 2][2];

void        I_XInputBindButton(int index, int key);
void        I_XInputPollEvent(void);
void        I_XInputReadActions(event_t *ev);
void        I_XInputVibrate(dboolean leftside, byte amount, int windDown);
void        I_XInputInit(void);
void        I_InitXInputCommands(void);

#endif // _USE_XINPUT

#endif // __I_XINPUT__