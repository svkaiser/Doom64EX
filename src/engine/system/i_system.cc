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
//
// DESCRIPTION: System Interface
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#endif

#include <stdarg.h>
#include <sys/stat.h>
#include <thread>
#include "doomstat.h"
#include "doomdef.h"
#include "m_misc.h"
#include "d_net.h"
#include "g_demo.h"
#include "d_main.h"
#include "con_console.h"
#include "z_zone.h"
#include "i_system.h"
#include "i_audio.h"
#include "gl_draw.h"

#include "SDL.h"

cvar::FloatVar i_gamma = 0.0;
cvar::FloatVar i_brightness = 100.0;
cvar::BoolVar i_interpolateframes = false;
cvar::StringVar s_soundfont = ""s;

namespace {
  std::chrono::steady_clock::time_point s_program_start{};

  uint32 s_get_ticks()
  {
      using namespace std::chrono;
      auto now = steady_clock::now();
      return duration_cast<milliseconds>(now - s_program_start).count();

  }
}

static int basetime = 0;

//
// I_Sleep
//
void I_Sleep(int usec)
{
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
}

//
// I_GetTimeNormal
//

static int I_GetTimeNormal(void)
{
    auto ticks = s_get_ticks();

    if (basetime == 0) {
        basetime = ticks;
    }

    ticks -= basetime;

    return (ticks * TICRATE) / 1000;
}

//
// I_GetTime_Error
//

static int I_GetTime_Error(void)
{
    I_Error("I_GetTime_Error: GetTime() used before initialization");
    return 0;
}

//
// I_InitClockRate
//

void I_InitClockRate(void)
{
    I_GetTime = I_GetTimeNormal;
}

//
// FRAME INTERPOLTATION
//

static unsigned int start_displaytime;
static unsigned int displaytime;
static dboolean InDisplay = false;

dboolean realframe = false;

fixed_t rendertic_frac = 0;
unsigned int rendertic_start;
unsigned int rendertic_step;
unsigned int rendertic_next;
const float rendertic_msec = 100 * TICRATE / 100000.0f;

//
// I_StartDisplay
//

dboolean I_StartDisplay(void)
{
    rendertic_frac = I_GetTimeFrac();

    if (InDisplay) {
        return false;
    }

    start_displaytime = s_get_ticks();
    InDisplay = true;

    return true;
}

//
// I_EndDisplay
//

void I_EndDisplay(void)
{
    displaytime = s_get_ticks() - start_displaytime;
    InDisplay = false;
}

//
// I_GetTimeFrac
//

fixed_t I_GetTimeFrac(void)
{
    unsigned long now;
    fixed_t frac;

    now = s_get_ticks();

    if (rendertic_step == 0) {
        return FRACUNIT;
    } else {
        frac = (fixed_t) ((now - rendertic_start + displaytime) * FRACUNIT / rendertic_step);
        if (frac < 0) {
            frac = 0;
        }
        if (frac > FRACUNIT) {
            frac = FRACUNIT;
        }
        return frac;
    }
}

//
// I_GetTime_SaveMS
//

void I_GetTime_SaveMS(void)
{
    rendertic_start = s_get_ticks();
    rendertic_next = (unsigned int) ((rendertic_start * rendertic_msec + 1.0f) / rendertic_msec);
    rendertic_step = rendertic_next - rendertic_start;
}

/**
 * @brief Get the user-writeable directory.
 *
 * Assume this is the only user-writeable directory on the system.
 *
 * @return Fully-qualified path that ends with a separator or NULL if not found.
 * @note The returning value MUST be freed by the caller.
 */

char *I_GetUserDir(void)
{
#ifdef _WIN32
    return I_GetBaseDir();
#else
    return SDL_GetPrefPath("", "doom64ex");
#endif
}

/**
 * @brief Get the directory which contains this program.
 *
 * @return Fully-qualified path that ends with a separator or NULL if not found.
 * @note The returning value MUST be freed by the caller.
 */

char *I_GetBaseDir(void)
{
    return SDL_GetBasePath();
}

/**
 * @brief Find a regular file in the user-writeable directory.
 *
 * @return Fully-qualified path or NULL if not found.
 * @note The returning value MUST be freed by the caller.
 */

char *I_GetUserFile(const char *file)
{
    char *path;
    char *userdir;

    if (!(userdir = I_GetUserDir()))
        return NULL;

    path = (char *) malloc(512);

    snprintf(path, 511, "%s%s", userdir, file);

    return path;
}

//
// I_GetTime
// returns time in 1/70th second tics
// now 1/35th?
//

int (*I_GetTime)(void) = I_GetTime_Error;

//
// I_GetTimeMS
//
// Same as I_GetTime, but returns time in milliseconds
//

int I_GetTimeMS(void)
{
    uint32 ticks;

    ticks = s_get_ticks();

    if (basetime == 0) {
        basetime = ticks;
    }

    return ticks - basetime;
}

//
// I_GetRandomTimeSeed
//

unsigned long I_GetRandomTimeSeed(void)
{
    // not exactly random....
    return s_get_ticks();
}

//
// I_Init
//

void I_Init(void)
{
    cvar::Register()
        (i_gamma, "i_Gamma", "")
        (i_brightness, "i_Brightness", "Brightness")
        (i_interpolateframes, "i_InterpolateFrames", "TODO")
        (s_soundfont, "s_SoundFont", "Path to 'doomsnd.sf2'");

    i_gamma.set_callback([](const float &) {
        void GL_DumpTextures();
        GL_DumpTextures();
    });

    i_brightness.set_callback([](const float &) {
        R_RefreshBrightness();
    });

    void imp_init_sdl2();
    imp_init_sdl2();
    
    I_InitClockRate();
}

//
// I_Quit
//

void I_Quit(void)
{
    if (demorecording) {
        endDemo = true;
        G_CheckDemoStatus();
    }

    M_SaveDefaults();

#ifdef USESYSCONSOLE
    // I_DestroySysConsole();
#endif

    I_ShutdownSound();

    void imp_quit_sdl2();
    imp_quit_sdl2();

    exit(0);
}

//
// I_BeginRead
//

dboolean inshowbusy = false;

void I_BeginRead(void)
{
    if (!devparm) {
        return;
    }

    if (inshowbusy) {
        return;
    }
    inshowbusy = true;
    inshowbusy = false;
    BusyDisk = true;
}

//
// I_RegisterCvars
//

#ifdef _USE_XINPUT
CVAR_EXTERNAL(i_rsticksensitivity);
CVAR_EXTERNAL(i_rstickthreshold);
CVAR_EXTERNAL(i_xinputscheme);
#endif

void I_RegisterCvars(void)
{
#ifdef _USE_XINPUT
    CON_CvarRegister(&i_rsticksensitivity);
    CON_CvarRegister(&i_rstickthreshold);
    CON_CvarRegister(&i_xinputscheme);
#endif
}
