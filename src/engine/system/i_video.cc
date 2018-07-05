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

#include "i_video.h"
#include <imp/Video>
#include <common/doomstat.h>

void I_UpdateGrab(void);

//================================================================================
// Video
//================================================================================

int video_width;
int video_height;
float video_ratio;

//
// I_InitScreen
//

void init_video_sdl();
void I_InitScreen(void) {
    init_video_sdl();

    auto mode = Video->current_mode();

    video_width = mode.width;
    video_height = mode.height;
    video_ratio = static_cast<float>(video_width) / video_height;

    // if(M_CheckParm("-window")) {
    //     InWindow = true;
    // }
    // if(M_CheckParm("-fullscreen")) {
    //     InWindow = false;
    // }

    // newwidth = newheight = 0;

    // p = M_CheckParm("-width");
    // if(p && p < myargc - 1) {
    //     newwidth = datoi(myargv[p+1]);
    // }

    // p = M_CheckParm("-height");
    // if(p && p < myargc - 1) {
    //     newheight = datoi(myargv[p+1]);
    // }
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
    delete Video;
}

//
// I_InitVideo
//

void I_InitVideo(void) {
    SDL_ShowCursor(0);
    I_InitScreen();
}

//
// I_StartTic
//

void I_StartTic(void) {
    Video->poll_events();
}

//
// I_FinishUpdate
//

extern bool BusyDisk;
void I_FinishUpdate(void) {
    I_UpdateGrab();
    Video->swap_window();

    BusyDisk = false;
}

//================================================================================
// Input
//================================================================================

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
      Video->grab(true);
	}

	if (!grab && currently_grabbed) {
      Video->grab(false);
	}

	currently_grabbed = grab;
}
