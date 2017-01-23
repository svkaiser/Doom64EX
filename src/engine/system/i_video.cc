// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2005 Simon Howard
// Copyright(C) 2007-2014 Samuel Villarreal
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
// DESCRIPTION:
//    SDL Stuff
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "SDL.h"

#include "m_misc.h"
#include "doomdef.h"
#include "doomstat.h"
#include "i_system.h"
#include "i_video.h"
#include "d_main.h"
#include "gl_main.h"

SDL_Window      *window;
SDL_GLContext   glContext;

FloatProperty v_msensitivityx("v_msensitivityy", "", 5.0f);
FloatProperty v_msensitivityy("v_msensitivityx", "", 5.0f);
FloatProperty v_macceleration("v_macceleration", "");
BoolProperty v_mlook("v_mlook", "");
BoolProperty v_mlookinvert("v_mlookinvert", "");
BoolProperty v_yaxismove("v_yaxismove", "");
IntProperty v_width("v_width", "", 640);
IntProperty v_height("v_height", "", 480);
BoolProperty v_windowed("v_windowed", "", true);
BoolProperty v_vsync("v_vsync", "", true);
IntProperty v_depthsize("v_depthsize", "", 24);
IntProperty v_buffersize("v_buffersize", "", 32);

extern BoolProperty m_menumouse;

static void I_GetEvent(SDL_Event *Event);
static void I_ReadMouse(void);
static void I_InitInputs(void);
void I_UpdateGrab(void);

//================================================================================
// Video
//================================================================================

SDL_Surface *screen;
int video_width;
int video_height;
float video_ratio;
dboolean window_focused;
dboolean window_mouse;

int mouse_x = 0;
int mouse_y = 0;

//
// I_InitScreen
//

void I_InitScreen(void) {
    int     newwidth;
    int     newheight;
    int     p;
    uint32  flags = 0;
    char    title[256];

    InWindow        = *v_windowed;
    video_width     = *v_width;
    video_height    = *v_height;
    video_ratio     = (float)video_width / (float)video_height;

    if(M_CheckParm("-window")) {
        InWindow = true;
    }
    if(M_CheckParm("-fullscreen")) {
        InWindow = false;
    }

    newwidth = newheight = 0;

    p = M_CheckParm("-width");
    if(p && p < myargc - 1) {
        newwidth = datoi(myargv[p+1]);
    }

    p = M_CheckParm("-height");
    if(p && p < myargc - 1) {
        newheight = datoi(myargv[p+1]);
    }

    if(newwidth && newheight) {
        video_width = newwidth;
        video_height = newheight;
        v_width = video_width;
        v_height = video_height;
    }

    if( v_depthsize != 8 &&
        v_depthsize != 16 &&
        v_depthsize != 24) {
        v_depthsize = 24;
    }

    if( v_buffersize != 8 &&
        v_buffersize != 16 &&
        v_buffersize != 24 &&
        v_buffersize != 32) {
        v_buffersize = 32;
    }

    usingGL = false;

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, *v_buffersize);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, *v_depthsize);
    SDL_GL_SetSwapInterval(v_vsync ? SDL_TRUE : SDL_FALSE);

    flags |= SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS;

    if(!InWindow) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    sprintf(title, "Doom64 - Version Date: %s", version_date);
    window = SDL_CreateWindow(title,
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              video_width,
                              video_height,
                              flags);

    if(window == NULL) {
        I_Error("I_InitScreen: Failed to create window");
        return;
    }

    if((glContext = SDL_GL_CreateContext(window)) == NULL) {
        // re-adjust depth size if video can't run it
        if(v_depthsize >= 24) {
            v_depthsize = 16;
        }
        else if(v_depthsize >= 16) {
            v_depthsize = 8;
        }

        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, *v_depthsize);

        if((glContext = SDL_GL_CreateContext(window)) == NULL) {
            // fall back to lower buffer setting
            v_buffersize = 16;
            SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, *v_buffersize);

            if((glContext = SDL_GL_CreateContext(window)) == NULL) {
                // give up
                I_Error("I_InitScreen: Failed to create OpenGL context");
                return;
            }
        }
    }
}

//
// I_ShutdownWait
//

int I_ShutdownWait(void) {
    static SDL_Event event;

    while(SDL_PollEvent(&event)) {
        if(event.type == SDL_QUIT ||
                (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
            I_ShutdownVideo();
#ifndef USESYSCONSOLE
            exit(0);
#else
            return 1;
#endif
        }
    }

    return 0;
}

//
// I_ShutdownVideo
//

void I_ShutdownVideo(void) {
    if(glContext) {
        SDL_GL_DeleteContext(glContext);
        glContext = NULL;
    }

    if(window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }

    SDL_Quit();
}

//
// I_InitVideo
//

void I_InitVideo(void) {
    uint32 f = SDL_INIT_VIDEO;

#ifdef _DEBUG
    f |= SDL_INIT_NOPARACHUTE;
#endif

    if(SDL_Init(f) < 0) {
        I_Error("ERROR - Failed to initialize SDL");
        return;
    }

    SDL_ShowCursor(0);
    I_InitInputs();
    I_InitScreen();
}

//
// I_StartTic
//

void I_StartTic(void) {
    SDL_Event Event;

    while(SDL_PollEvent(&Event)) {
        I_GetEvent(&Event);
    }

#ifdef _USE_XINPUT
    I_XInputPollEvent();
#endif

    I_ReadMouse();
}

//
// I_FinishUpdate
//

void I_FinishUpdate(void) {
    I_UpdateGrab();
    SDL_GL_SwapWindow(window);

    BusyDisk = false;
}

//================================================================================
// Input
//================================================================================

float mouse_accelfactor;

int         UseJoystick;
dboolean    DigiJoy;
int         DualMouse;

dboolean    MouseMode;//false=microsoft, true=mouse systems

//
// I_TranslateKey
//

static int I_TranslateKey(const int key) {
    int rc = 0;

    switch(key) {
    case SDLK_LEFT:
        rc = KEY_LEFTARROW;
        break;
    case SDLK_RIGHT:
        rc = KEY_RIGHTARROW;
        break;
    case SDLK_DOWN:
        rc = KEY_DOWNARROW;
        break;
    case SDLK_UP:
        rc = KEY_UPARROW;
        break;
    case SDLK_ESCAPE:
        rc = KEY_ESCAPE;
        break;
    case SDLK_RETURN:
        rc = KEY_ENTER;
        break;
    case SDLK_TAB:
        rc = KEY_TAB;
        break;
    case SDLK_F1:
        rc = KEY_F1;
        break;
    case SDLK_F2:
        rc = KEY_F2;
        break;
    case SDLK_F3:
        rc = KEY_F3;
        break;
    case SDLK_F4:
        rc = KEY_F4;
        break;
    case SDLK_F5:
        rc = KEY_F5;
        break;
    case SDLK_F6:
        rc = KEY_F6;
        break;
    case SDLK_F7:
        rc = KEY_F7;
        break;
    case SDLK_F8:
        rc = KEY_F8;
        break;
    case SDLK_F9:
        rc = KEY_F9;
        break;
    case SDLK_F10:
        rc = KEY_F10;
        break;
    case SDLK_F11:
        rc = KEY_F11;
        break;
    case SDLK_F12:
        rc = KEY_F12;
        break;
    case SDLK_BACKSPACE:
        rc = KEY_BACKSPACE;
        break;
    case SDLK_DELETE:
        rc = KEY_DEL;
        break;
    case SDLK_INSERT:
        rc = KEY_INSERT;
        break;
    case SDLK_PAGEUP:
        rc = KEY_PAGEUP;
        break;
    case SDLK_PAGEDOWN:
        rc = KEY_PAGEDOWN;
        break;
    case SDLK_HOME:
        rc = KEY_HOME;
        break;
    case SDLK_END:
        rc = KEY_END;
        break;
    case SDLK_PAUSE:
        rc = KEY_PAUSE;
        break;
    case SDLK_EQUALS:
        rc = KEY_EQUALS;
        break;
    case SDLK_MINUS:
        rc = KEY_MINUS;
        break;
	case SDLK_KP_0:
        rc = KEY_KEYPAD0;
        break;
	case SDLK_KP_1:
        rc = KEY_KEYPAD1;
        break;
	case SDLK_KP_2:
        rc = KEY_KEYPAD2;
        break;
	case SDLK_KP_3:
        rc = KEY_KEYPAD3;
        break;
	case SDLK_KP_4:
        rc = KEY_KEYPAD4;
        break;
	case SDLK_KP_5:
        rc = KEY_KEYPAD5;
        break;
	case SDLK_KP_6:
        rc = KEY_KEYPAD6;
        break;
	case SDLK_KP_7:
        rc = KEY_KEYPAD7;
        break;
	case SDLK_KP_8:
        rc = KEY_KEYPAD8;
        break;
	case SDLK_KP_9:
        rc = KEY_KEYPAD9;
        break;
    case SDLK_KP_PLUS:
        rc = KEY_KEYPADPLUS;
        break;
    case SDLK_KP_MINUS:
        rc = KEY_KEYPADMINUS;
        break;
    case SDLK_KP_DIVIDE:
        rc = KEY_KEYPADDIVIDE;
        break;
    case SDLK_KP_MULTIPLY:
        rc = KEY_KEYPADMULTIPLY;
        break;
    case SDLK_KP_ENTER:
        rc = KEY_KEYPADENTER;
        break;
    case SDLK_KP_PERIOD:
        rc = KEY_KEYPADPERIOD;
		break;
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
        rc = KEY_RSHIFT;
        break;
    case SDLK_LCTRL:
    case SDLK_RCTRL:
        rc = KEY_RCTRL;
        break;
    case SDLK_LALT:
    //case SDLK_LMETA:
    case SDLK_RALT:
    //case SDLK_RMETA:
        rc = KEY_RALT;
        break;
    case SDLK_CAPSLOCK:
        rc = KEY_CAPS;
        break;
    default:
        rc = key;
        break;
    }

    return rc;

}

//
// I_SDLtoDoomMouseState
//

static int I_SDLtoDoomMouseState(Uint8 buttonstate) {
    return 0
           | (buttonstate & SDL_BUTTON(SDL_BUTTON_LEFT)      ? 1 : 0)
           | (buttonstate & SDL_BUTTON(SDL_BUTTON_MIDDLE)    ? 2 : 0)
           | (buttonstate & SDL_BUTTON(SDL_BUTTON_RIGHT)     ? 4 : 0);
}

//
// I_ReadMouse
//

static void I_ReadMouse(void) {
    int x, y;
    Uint8 btn;
    event_t ev;
    static Uint8 lastmbtn = 0;

    SDL_GetRelativeMouseState(&x, &y);
    btn = SDL_GetMouseState(&mouse_x, &mouse_y);

    if(x != 0 || y != 0 || btn || (lastmbtn != btn)) {
        ev.type = ev_mouse;
        ev.data1 = I_SDLtoDoomMouseState(btn);
        ev.data2 = x << 5;
        ev.data3 = (-y) << 5;
        ev.data4 = 0;
        D_PostEvent(&ev);
    }

	lastmbtn = btn;
}

//
// I_MouseAccelChange
//

void I_MouseAccelChange(void) {
    mouse_accelfactor = v_macceleration / 200.0f + 1.0f;
}

//
// I_MouseAccel
//

int I_MouseAccel(int val) {
    if(!v_macceleration) {
        return val;
    }

    if(val < 0) {
        return -I_MouseAccel(-val);
    }

    return (int)(pow((double)val, (double)mouse_accelfactor));
}

//
// I_ActivateMouse
//

static void I_ActivateMouse(void) {
    SDL_ShowCursor(0);
    SDL_SetRelativeMouseMode(SDL_TRUE);
}

//
// I_DeactivateMouse
//

static void I_DeactivateMouse(void) {
    SDL_ShowCursor(m_menumouse ? SDL_FALSE : SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_FALSE);
}

//
// I_UpdateGrab
//

void I_UpdateGrab(void) {
    static dboolean currently_grabbed = false;
	dboolean grab;

	grab = /*window_mouse &&*/ !menuactive
	&& (gamestate == GS_LEVEL)
	&& !demoplayback;

	if (grab && !currently_grabbed) {
		SDL_ShowCursor(0);
		SDL_SetRelativeMouseMode(SDL_TRUE);
		SDL_SetWindowGrab(window, SDL_TRUE);
	}

	if (!grab && currently_grabbed) {
		SDL_SetRelativeMouseMode(SDL_FALSE);
		SDL_SetWindowGrab(window, SDL_FALSE);
		SDL_ShowCursor(m_menumouse ? SDL_FALSE : SDL_TRUE);
	}

	currently_grabbed = grab;
}

//
// I_GetEvent
//

void CON_ToggleConsole(void);

static void I_GetEvent(SDL_Event *Event) {
    event_t event;
    uint32 mwheeluptic = 0, mwheeldowntic = 0;
    uint32 tic = gametic;

    switch(Event->type) {
    case SDL_KEYDOWN:
        if(Event->key.repeat)
            break;

        if (Event->key.keysym.scancode == SDL_SCANCODE_GRAVE) {
            CON_ToggleConsole();
            break;
        }

        event.type = ev_keydown;
        event.data1 = I_TranslateKey(Event->key.keysym.sym);
        D_PostEvent(&event);
    break;

    case SDL_KEYUP:
        event.type = ev_keyup;
        event.data1 = I_TranslateKey(Event->key.keysym.sym);
        D_PostEvent(&event);
        break;

  case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		if (!window_focused)
			break;

		event.type = (Event->type == SDL_MOUSEBUTTONUP) ? ev_mouseup : ev_mousedown;
		event.data1 =
			I_SDLtoDoomMouseState(SDL_GetMouseState(NULL, NULL));
		event.data2 = event.data3 = 0;

		D_PostEvent(&event);
		break;

	case SDL_MOUSEWHEEL:
		if (Event->wheel.y > 0) {
			event.type = ev_keydown;
			event.data1 = KEY_MWHEELUP;
			mwheeluptic = tic;
		} else if (Event->wheel.y < 0) {
			event.type = ev_keydown;
			event.data1 = KEY_MWHEELDOWN;
			mwheeldowntic = tic;
		} else
			break;

		event.data2 = event.data3 = 0;
		D_PostEvent(&event);
		break;

	case SDL_WINDOWEVENT:
		switch (Event->window.event) {
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			window_focused = true;
			break;

		case SDL_WINDOWEVENT_FOCUS_LOST:
			window_focused = false;
			break;

		case SDL_WINDOWEVENT_ENTER:
			window_mouse = true;
			break;

		case SDL_WINDOWEVENT_LEAVE:
			window_mouse = false;
			break;

		default:
			break;
		}
		break;

    case SDL_QUIT:
        I_Quit();
        break;

    default:
        break;
    }

    if(mwheeluptic && mwheeluptic + 1 < tic) {
        event.type = ev_keyup;
        event.data1 = KEY_MWHEELUP;
        D_PostEvent(&event);
        mwheeluptic = 0;
    }

    if(mwheeldowntic && mwheeldowntic + 1 < tic) {
        event.type = ev_keyup;
        event.data1 = KEY_MWHEELDOWN;
        D_PostEvent(&event);
        mwheeldowntic = 0;
    }
}

//
// I_InitInputs
//

static void I_InitInputs(void) {
	SDL_PumpEvents();

    SDL_ShowCursor(m_menumouse ? SDL_FALSE : SDL_TRUE);

    I_MouseAccelChange();

#ifdef _USE_XINPUT
    I_XInputInit();
#endif
}
