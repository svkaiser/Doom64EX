// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2009 James Haley
//
// Derived from PrBoom
// Copyright 2006 Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//----------------------------------------------------------------------------
//
// DESCRIPTION:
//
//   CPU-related system-specific module for Windows only
//
//-----------------------------------------------------------------------------

#ifndef _WIN32
#error This version of i_cpu.c is for Win32 only.
#endif

#include <stdio.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "doomtype.h"
#include "con_cvar.h"

// cannot include DOOM headers here; required externs:
extern void     I_Printf(char* string, ...);
extern int      M_CheckParm(const char *);
extern int      datoi(const char *str);
extern int      myargc;
extern char**   myargv;

unsigned int process_affinity_mask;

// haleyjd 05/12/09: Load SetAffinityProcessMask dynamically to avoid
// problems on Win9x systems. Although it's in the 6.0 headers, this
// function apparently only exists on NT kernels. Win9x never supported
// multiple processors anyway so I fail to see the necessity of even
// calling the function there if it did exist.

typedef BOOL (WINAPI *SetAffinityFunc)(HANDLE, DWORD);

//
// I_SetAffinityMask
//
// Due to ongoing problems with the SDL_mixer library and/or the SDL audio
// core, it is necessary to support the restriction of Eternity to a single
// core or CPU. Apparent thread contention issues cause the library to
// malfunction if multiple threads of the process run simultaneously.
//
// I wish SDL would fix their bullshit already.
//
void I_SetAffinityMask(void) {
    int p;
    HMODULE kernel32_dll;
    SetAffinityFunc SetAffinity;

    // Find the kernel interface DLL.

    kernel32_dll = LoadLibrary("kernel32.dll");

    if(kernel32_dll == NULL) {
        // This should never happen...
        fprintf(stderr, "Failed to load kernel32.dll\n");
        return;
    }

    // Find the SetProcessAffinityMask function.
    SetAffinity =
        (SetAffinityFunc)GetProcAddress(kernel32_dll, "SetProcessAffinityMask");

    if(!SetAffinity) {
        I_Printf("I_SetAffinityMask: system does not support multiple CPUs.\n");
        return;
    }

    p = M_CheckParm("-affinity");
    if(p && p < myargc - 1) {
        process_affinity_mask = datoi(myargv[p + 1]);
    }
    else {
        process_affinity_mask = 0;
    }

    // Set the process affinity mask so that all threads
    // run on the same processor.  This is a workaround for a bug in
    // SDL_mixer that causes occasional crashes.
    if(process_affinity_mask) {
        if(!SetAffinity(GetCurrentProcess(), process_affinity_mask)) {
            I_Printf("I_SetAffinityMask: failed to set process affinity mask.\n");
        }
        else {
            I_Printf("I_SetAffinityMask: applied affinity mask.\n");
        }
    }
}

// EOF

