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
//
// DESCRIPTION: XInput API Code
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include "doomtype.h"
#include "doomstat.h"
#include "doomdef.h"
#include "i_xinput.h"
#include "d_main.h"
#include "g_controls.h"
#include "d_event.h"
#include "con_cvar.h"
#include "m_misc.h"

#ifdef _USE_XINPUT

xinputgamepad_t xgamepad;

//
// Library functions
//

typedef void (WINAPI* LPXINPUTENABLE)(dboolean enable);
static LPXINPUTENABLE   I_XInputEnable      = NULL;

typedef dword(WINAPI* LPXINPUTGETSTATE)(dword userID, xinputstate_t* state);
static LPXINPUTGETSTATE I_XInputGetState    = NULL;

typedef dword(WINAPI* LPXINPUTSETRUMBLE)(dword userID, xinputrumble_t* rumble);
static LPXINPUTSETRUMBLE I_XInputSetRumble  = NULL;

CVAR(i_rsticksensitivity, 0.0080);
CVAR(i_rstickthreshold, 20.0);
CVAR(i_xinputscheme, 0);

//
// I_XInputClampDeadZone
//

static void I_XInputClampDeadZone(void) {
    xinputbuttons_t* buttons;

    buttons = &xgamepad.state.buttons;

    if(buttons->lx < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
            buttons->lx > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        buttons->lx = 0;
    }

    if(buttons->ly < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
            buttons->ly > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        buttons->ly = 0;
    }

    if(buttons->rx < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
            buttons->rx > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
        buttons->rx = 0;
    }

    if(buttons->ry < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
            buttons->ry > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
        buttons->ry = 0;
    }
}

//
// I_XInputPollEvent
//

void I_XInputPollEvent(void) {
    xinputbuttons_t* buttons;
    event_t event;
    dboolean ltrigger;
    dboolean rtrigger;
    int i;
    int j;
    int bits;

    //
    // is xinput api even available?
    //
    if(!xgamepad.available) {
        return;
    }

    dmemset(&event, 0, sizeof(event_t));

    //
    // clamp cvar values
    //
    if(i_rsticksensitivity.value < 0.001f) {
        CON_CvarSetValue(i_rsticksensitivity.name, 0.001f);
    }

    if(i_rstickthreshold.value < 1.0f) {
        CON_CvarSetValue(i_rstickthreshold.name, 1.0f);
    }

    //
    // fetch current state and
    // check if controller is still connected
    //
    if(I_XInputGetState(0, &xgamepad.state)) {
        xgamepad.connected = false;
        return;
    }
    else {
        xgamepad.connected = true;
    }

    //
    // ramp down vibration speed
    //
    if(xgamepad.lMotorWindDown || xgamepad.rMotorWindDown) {
        int temp;

        temp = xgamepad.vibration.lMotorSpeed;
        xgamepad.vibration.lMotorSpeed = MAX(temp - xgamepad.lMotorWindDown, 0);

        temp = xgamepad.vibration.rMotorSpeed;
        xgamepad.vibration.rMotorSpeed = MAX(temp - xgamepad.rMotorWindDown, 0);

        I_XInputSetRumble(0, &xgamepad.vibration);
    }

    //
    // clamp stick threshold
    //
    I_XInputClampDeadZone();

    buttons = &xgamepad.state.buttons;
    ltrigger = buttons->ltrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    rtrigger = buttons->rtrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

    //
    // read left stick
    //
    if(buttons->lx != 0 || buttons->ly != 0) {
        //
        // hack for classic scheme
        //
        if(i_xinputscheme.value && buttons->lx == 0) {
            xgamepad.rxthreshold = 0.0f;
        }

        //
        // set event data
        //
        event.type = ev_gamepad;
        event.data1 = buttons->lx;
        event.data2 = buttons->ly;
        event.data3 = XINPUT_GAMEPAD_LEFT_STICK;
        event.data4 = buttons->data;
        D_PostEvent(&event);
    }
    else if(i_xinputscheme.value) {
        xgamepad.rxthreshold = 0.0f;
        xgamepad.rythreshold = 0.0f;
    }

    //
    // read right stick
    //
    if(buttons->rx != 0 || buttons->ry != 0) {
        event.type = ev_gamepad;
        event.data1 = buttons->rx;
        event.data2 = buttons->ry;
        event.data3 = XINPUT_GAMEPAD_RIGHT_STICK;
        event.data4 = buttons->data;
        D_PostEvent(&event);
    }
    else if(!i_xinputscheme.value) {
        xgamepad.rxthreshold = 0.0f;
        xgamepad.rythreshold = 0.0f;
    }

    //
    // read buttons
    //
    if((buttons->data || (ltrigger || rtrigger))) {
        bits = buttons->data;

        //
        // left/right triggers isn't classified as buttons
        // but treat them like buttons here
        //

        if(ltrigger) {
            bits |= XINPUT_GAMEPAD_LEFT_TRIGGER;
        }

        if(rtrigger) {
            bits |= XINPUT_GAMEPAD_RIGHT_TRIGGER;
        }

        // villsa 01052014 -  check for button press
        bits &= ~xgamepad.oldbuttons;
        for(j = 0, i = 1; i != 0x100000; i <<= 1) {
            if(!(i & 0xFF3FF)) {
                continue;
            }

            if(bits & i) {
                event.type = ev_keydown;
                event.data1 = BUTTON_DPAD_UP + j;
                D_PostEvent(&event);
            }

            j++;
        }
    }

    bits = xgamepad.oldbuttons;
    xgamepad.oldbuttons = buttons->data;

    if(ltrigger) {
        xgamepad.oldbuttons |= XINPUT_GAMEPAD_LEFT_TRIGGER;
    }

    if(rtrigger) {
        xgamepad.oldbuttons |= XINPUT_GAMEPAD_RIGHT_TRIGGER;
    }

    // villsa 01052014 -  check for button release
    bits &= ~xgamepad.oldbuttons;
    for(j = 0, i = 1; i != 0x100000; i <<= 1) {
        if(!(i & 0xFF3FF)) {
            continue;
        }

        if(bits & i) {
            event.type = ev_keyup;
            event.data1 = BUTTON_DPAD_UP + j;
            D_PostEvent(&event);
        }

        j++;
    }
}

//
// I_XInputVibrate
//

void I_XInputVibrate(dboolean leftside, byte amount, int windDown) {
    if(!xgamepad.connected) {
        return;
    }

    if(leftside) {
        xgamepad.vibration.lMotorSpeed = amount * 0xff;
        xgamepad.lMotorWindDown = windDown;
    }
    else {
        xgamepad.vibration.rMotorSpeed = amount * 0xff;
        xgamepad.rMotorWindDown = windDown;
    }

    I_XInputSetRumble(0, &xgamepad.vibration);
}

//
// I_XInputReadActions
// Read inputs for in-game player
//

void I_XInputReadActions(event_t *ev) {
    playercontrols_t *pc = &Controls;

    if(ev->type == ev_gamepad) {
        //
        // left analog stick
        //
        if(ev->data3 == XINPUT_GAMEPAD_LEFT_STICK) {
            float x;
            float y;

            pc->flags |= PCF_GAMEPAD;

            y = (float)ev->data2 * 0.0015f;

            //
            // classic scheme uses the traditional turning for x-axis
            //
            if(i_xinputscheme.value > 0) {
                float turnspeed;

                turnspeed = i_rsticksensitivity.value / i_rstickthreshold.value;

                if(ev->data1 != 0) {
                    xgamepad.rxthreshold += turnspeed;

                    if(xgamepad.rxthreshold >= i_rsticksensitivity.value) {
                        xgamepad.rxthreshold = i_rsticksensitivity.value;
                    }
                }

                x = (float)ev->data1 * xgamepad.rxthreshold;
                pc->mousex += (int)x;
            }
            //
            // modern scheme uses strafing for x-axis
            //
            else {
                x = (float)ev->data1 * 0.0015f;
                pc->joyx += (int)x;
            }

            pc->joyy += (int)y;

            return;
        }

        //
        // right analog stick
        //
        if(ev->data3 == XINPUT_GAMEPAD_RIGHT_STICK) {
            float x;
            float y;
            float turnspeed;

            turnspeed = i_rsticksensitivity.value / i_rstickthreshold.value;

            if(ev->data1 != 0) {
                xgamepad.rxthreshold += turnspeed;

                if(xgamepad.rxthreshold >= i_rsticksensitivity.value) {
                    xgamepad.rxthreshold = i_rsticksensitivity.value;
                }
            }

            if(ev->data2 != 0) {
                xgamepad.rythreshold += turnspeed;

                if(xgamepad.rythreshold >= i_rsticksensitivity.value) {
                    xgamepad.rythreshold = i_rsticksensitivity.value;
                }
            }

            x = (float)ev->data1 * xgamepad.rxthreshold;
            y = (float)ev->data2 * xgamepad.rythreshold;

            pc->mousex += (int)x;
            pc->mousey += (int)y;

            return;
        }
    }
}

//
// I_XInputInit
// Initialize the xinput API
//

void I_XInputInit(void) {
    HINSTANCE hInst;

    dmemset(&xgamepad, 0, sizeof(xinputgamepad_t));

    //
    // check for disabling parameter
    //
    if(M_CheckParm("-noxinput")) {
        return;
    }

    //
    // locate xinput dynamic link library
    //
    if(hInst = LoadLibrary(XINPUT_DLL)) {
        //
        // get routines from module
        //
        I_XInputEnable      = (LPXINPUTENABLE)GetProcAddress(hInst, "XInputEnable");
        I_XInputGetState    = (LPXINPUTGETSTATE)GetProcAddress(hInst, "XInputGetState");
        I_XInputSetRumble   = (LPXINPUTSETRUMBLE)GetProcAddress(hInst, "XInputSetState");

        if(I_XInputEnable == NULL || I_XInputGetState == NULL || I_XInputSetRumble == NULL) {
            return;
        }

        I_XInputEnable(true);

        //
        // api is available
        //
        xgamepad.available = true;

        //
        // check if controller is plugged in
        //
        if(!(I_XInputGetState(0, &xgamepad.state))) {
            xgamepad.connected = true;
        }
    }
}

#endif  // _USE_XINPUT
