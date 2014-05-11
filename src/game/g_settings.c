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
//
// DESCRIPTION: Doom3D's config file parsing
//
//-----------------------------------------------------------------------------

#ifndef _WIN32
// 20120105 bkw: G_GetConfigFileName() needs these, better safe than sorry
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "g_local.h"
#include "z_zone.h"
#include "m_misc.h"
#include "con_console.h"
#include "i_system.h"

static char *ConfigFileName =
#ifdef _WIN32
    "config.cfg"
#else
    NULL
#endif
    ;

char    DefaultConfig[] =
#include "defconfig.inc"    // wtf?
    ;

//
// G_ExecuteMultipleCommands
//

char *G_GetConfigFileName(void) {
#ifdef _WIN32
    CON_DPrintf("Loading config: %s\\%s\n", I_DoomExeDir(), ConfigFileName);
    return ConfigFileName;
#else
    // 20120105 bkw: Be UNIX-friendly and use ~/.doom64ex/config.cfg
    if(ConfigFileName == NULL) {
        char confdir[PATH_MAX];
        static char conffile[PATH_MAX];

        char *homedir = getenv("HOME");

        if(!homedir) {
            homedir = ".";    // Fall back to ./.doom64ex if HOME not set
        }

        // make sure the directory exists
        sprintf(confdir, "%s/.doom64ex", homedir);
        (void)mkdir(confdir, 0755); // ignore return value

        sprintf(conffile, "%s/config.cfg", confdir);
        ConfigFileName = conffile;
        I_Printf("G_GetConfigFileName: using config file '%s'\n", ConfigFileName);
    }

    CON_DPrintf("Loading config: %s\n", ConfigFileName);
    return ConfigFileName;
#endif
}

void G_ExecuteMultipleCommands(char *data) {
    char    *p;
    char    *q;
    char    c;
    char    line[1024];

    p=data;
    c=*p;
    while(c) {
        q=line;
        c=*(p++);
        while(c&&(c!='\n')) {
            if(c!='\r') {
                *(q++)=c;
            }
            c=*(p++);
        }
        *q=0;
        if(line[0]) {
            G_ExecuteCommand(line);
        }
    }
}

//
// G_ExecuteFile
//

void G_ExecuteFile(char *name) {
    FILE    *fh;
    char    *buff;
    int        len;

    if(!name) {
        I_Error("G_ExecuteFile: No config name specified");
    }

    fh = fopen(name, "rb");

    if(!fh) {
        fh = fopen(name, "w");
        if(!fh) {
            I_Error("G_ExecuteFile: Unable to create %s", name);
        }

        fprintf(fh, "%s", DefaultConfig);
        fclose(fh);

        fh = fopen(name, "rb");

        if(!fh) {
            I_Error("G_ExecuteFile: Failed to read %s", name);
        }
    }

    fseek(fh, 0, SEEK_END);
    len = ftell(fh);
    fseek(fh, 0, SEEK_SET);
    buff = Z_Malloc(len + 1, PU_STATIC, NULL);
    fread(buff, 1, len, fh);
    buff[len] = 0;
    G_ExecuteMultipleCommands(buff);
    Z_Free(buff);
}

//
// G_LoadSettings
//

void G_LoadSettings(void) {
    int        p;

    p = M_CheckParm("-config");
    if(p && (p < myargc - 1)) {
        if(myargv[p + 1][0] != '-') {
            ConfigFileName = myargv[p + 1];
        }
    }

    G_ExecuteFile(G_GetConfigFileName());
}
