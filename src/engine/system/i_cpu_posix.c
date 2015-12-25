// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2009 James Haley
//
// Derived from Chocolate Doom
// Copyright 2009 Simon Howard
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
//   CPU-related system-specific module for POSIX systems
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_SCHED_SETAFFINITY
#include <unistd.h>
#include <sched.h>
#endif

#include "con_cvar.h"

// cannot include DOOM headers here; required externs:
extern void     I_Printf(char* string, ...);
extern int      M_CheckParm(const char *);
extern int      datoi(const char *str);
extern int      myargc;
extern char**   myargv;

unsigned int process_affinity_mask;

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
#ifdef HAVE_SCHED_SETAFFINITY
    int p = M_CheckParm("-affinity");

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
        int i;
        cpu_set_t set;

        CPU_ZERO(&set);

        for(i = 0; i < 16; ++i) {
            CPU_SET((process_affinity_mask>>i)&1, &set);
        }

        if(sched_setaffinity(getpid(), sizeof(set), &set) == -1) {
            I_Printf("I_SetAffinityMask: failed to set process affinity mask.\n");
        }
        else {
            I_Printf("I_SetAffinityMask: applied affinity mask.\n");
        }
    }
#endif
}

// EOF

