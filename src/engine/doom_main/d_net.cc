// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2005 Simon Howard
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
// DESCRIPTION:
//    DOOM Network game communication and protocol,
//    all OS independend parts.
//
//-----------------------------------------------------------------------------

#include "m_menu.h"
#include "i_system.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"
#include "tables.h"
#include "m_misc.h"
#include "con_console.h"
#include "SDL.h"
#include "i_video.h"

#define FEATURE_MULTIPLAYER 1

dboolean    ShowGun=true;
dboolean    drone = false;
dboolean    net_cl_new_sync = true;    // Use new client syncronisation code
fixed_t     offsetms;

//
// NETWORKING
//
// gametic is the tic about to (or currently being) run
// maketic is the tick that hasn't had control made for it yet
// nettics[] has the maketics for all players
//
// a gametic cannot be run until nettics[] > gametic for all players

ticcmd_t    netcmds[MAXPLAYERS][BACKUPTICS];
int         nettics[MAXNETNODES];
dboolean    nodeingame[MAXNETNODES];        // set false as nodes leave game
dboolean    remoteresend[MAXNETNODES];      // set when local needs tics
int         resendto[MAXNETNODES];          // set when remote needs tics
int         resendcount[MAXNETNODES];

int         nodeforplayer[MAXPLAYERS];

int         maketic=0;
int         lastnettic=0;
int         skiptics=0;
int         ticdup=0;
int         maxsend=0;    // BACKUPTICS/(2*ticdup)-1
int         extratics;


void D_ProcessEvents(void);
void G_BuildTiccmd(ticcmd_t *cmd);
void D_Display(void);

dboolean renderinframe = false;

//
// GetAdjustedTime
// 30 fps clock adjusted by offsetms milliseconds
//

static int GetAdjustedTime(void) {
    int time_ms;

    time_ms = I_GetTimeMS();

    if(net_cl_new_sync) {
        // Use the adjustments from net_client.c only if we are
        // using the new sync mode.

        time_ms += (offsetms / FRACUNIT);
    }

    return (time_ms * TICRATE) / 1000;
}

//
// NetUpdate
// Builds ticcmds for console player,
// sends out a packet
//
int gametime = 0;

void NetUpdate(void) {
    int nowtime;
    int newtics;
    int i;
    int gameticdiv;

    if(renderinframe) {
        return;
    }

#ifdef FEATURE_MULTIPLAYER

    // Run network subsystems

    NET_CL_Run();
    NET_SV_Run();

#endif

    // check time
    nowtime = GetAdjustedTime()/ticdup;
    newtics = nowtime - gametime;
    gametime = nowtime;

    if(skiptics <= newtics) {
        newtics -= skiptics;
        skiptics = 0;
    }
    else {
        skiptics -= newtics;
        newtics = 0;
    }

    // build new ticcmds for console player(s)
    gameticdiv = gametic/ticdup;
    for(i=0 ; i<newtics ; i++) {
        ticcmd_t cmd;

        I_StartTic();
        D_ProcessEvents();
        //if (maketic - gameticdiv >= BACKUPTICS/2-1)
        //    break;          // can't hold any more

        M_Ticker();

        if(drone) {  // In drone mode, do not generate any ticcmds.
            continue;
        }

        if(net_cl_new_sync) {
            // If playing single player, do not allow tics to buffer
            // up very far

            if((!netgame || demoplayback) && maketic - gameticdiv > 2) {
                break;
            }

            // Never go more than ~200ms ahead
            if(maketic - gameticdiv > 8) {
                break;
            }
        }
        else {
            if(maketic - gameticdiv >= 5) {
                break;
            }
        }

        G_BuildTiccmd(&cmd);

#ifdef FEATURE_MULTIPLAYER

        if(netgame && !demoplayback) {
            NET_CL_SendTiccmd(&cmd, maketic);
        }

#endif
        netcmds[consoleplayer][maketic % BACKUPTICS] = cmd;

        ++maketic;
        nettics[consoleplayer] = maketic;
    }
}

//
// D_StartGameLoop
//
// Called after the screen is set but before the game starts running.
//

void D_StartGameLoop(void) {
    gametime = GetAdjustedTime() / ticdup;
}

//
// PrintMD5Digest
//

static dboolean had_warning = false;
static void PrintMD5Digest(const char *s, byte *digest) {
    unsigned int i;

    I_Printf("%s: ", s);

    for(i = 0; i < sizeof(md5_digest_t); ++i) {
        I_Printf("%02x", digest[i]);
    }

    I_Printf("\n");
}

//
// CheckMD5Sums
//

static void CheckMD5Sums(void) {
    dboolean correct_wad;

    if(!net_client_received_wait_data || had_warning) {
        return;
    }

    correct_wad = memcmp(net_local_wad_md5sum,
                         net_server_wad_md5sum, sizeof(md5_digest_t)) == 0;

    if(correct_wad) {
        return;
    }
    else {
        I_Printf("Warning: WAD MD5 does not match server:\n");
        PrintMD5Digest("Local", net_local_wad_md5sum);
        PrintMD5Digest("Server", net_server_wad_md5sum);
        I_Printf("If you continue, this may cause your game to desync\n");
    }

    had_warning = true;
}

//
// D_NetWait
//

static void D_NetWait(void) {
    SDL_Event Event;
    unsigned int id = 0;

    if(M_CheckParm("-server") > 0) {
#ifdef USESYSCONSOLE
        I_Printf("D_NetWait: Waiting for players..\n\nPress ready key to begin game..\n\n");
#else
        I_Printf("D_NetWait: Waiting for players..\n\nWhen ready press any key to begin game..\n\n");
#endif
    }

    I_Printf("---------------------------------------------\n\n");

    while(net_waiting_for_start) {
        CheckMD5Sums();

        if(id != net_clients_in_game) {
            I_Printf("%s - %s\n", net_player_names[net_clients_in_game-1],
                     net_player_addresses[net_clients_in_game-1]);
            id = net_clients_in_game;
        }

        NET_CL_Run();
        NET_SV_Run();

        if(!net_client_connected) {
            I_Error("D_NetWait: Disconnected from server");
        }

        I_Sleep(100);

        while(SDL_PollEvent(&Event))
            if(Event.type == SDL_KEYDOWN && Event.key.keysym.sym == SDLK_RETURN) {
                NET_CL_StartGame();
            }
    }
}

//
// D_CheckNetGame
// Works out player numbers among the net participants
//

void D_CheckNetGame(void) {
    int    i;
    int    num_players;

    // default values for single player

    consoleplayer = 0;
    netgame = false;
    ticdup = 1;
    extratics = 1;
    offsetms = 0;

    for(i = 0; i < MAXPLAYERS; i++) {
        playeringame[i] = false;
        nettics[i] = 0;
    }

    playeringame[0] = true;

#ifdef FEATURE_MULTIPLAYER
    {
        net_addr_t *addr = NULL;

        //!
        // @category net
        //
        // Start a multiplayer server, listening for connections.
        //

        if(M_CheckParm("-server") > 0) {
            NET_SV_Init();
            NET_SV_AddModule(&net_loop_server_module);
            NET_SV_AddModule(&net_sdl_module);

            net_loop_client_module.InitClient();
            addr = net_loop_client_module.ResolveAddress(NULL);
        }
        else {
            //!
            // @category net
            //
            // Automatically search the local LAN for a multiplayer
            // server and join it.
            //

            i = M_CheckParm("-autojoin");

            if(i > 0) {
                addr = NET_FindLANServer();

                if(addr == NULL) {
                    I_Error("No server found on local LAN");
                }
            }

            //!
            // @arg <address>
            // @category net
            //
            // Connect to a multiplayer server running on the given
            // address.
            //

            i = M_CheckParm("-connect");

            if(i > 0) {
                net_sdl_module.InitClient();
                addr = net_sdl_module.ResolveAddress(myargv[i+1]);

                if(addr == NULL) {
                    I_Error("Unable to resolve '%s'\n", myargv[i+1]);
                }
            }
        }

        if(addr != NULL) {
            if(M_CheckParm("-drone") > 0) {
                drone = true;
            }

            //!
            // @category net
            //
            // Run as the left screen in three screen mode.
            //

            if(M_CheckParm("-left") > 0) {
                viewangleoffset = ANG90;
                drone = true;
            }

            //!
            // @category net
            //
            // Run as the right screen in three screen mode.
            //

            if(M_CheckParm("-right") > 0) {
                viewangleoffset = ANG270;
                drone = true;
            }

            if(!NET_CL_Connect(addr)) {
                I_Error("D_CheckNetGame: Failed to connect to %s\n",
                        NET_AddrToString(addr));
            }

            I_Printf("D_CheckNetGame: Connected to %s\n", NET_AddrToString(addr));

            D_NetWait();
        }
    }

#endif

    num_players = 0;

    for(i = 0; i < MAXPLAYERS; ++i) {
        if(playeringame[i]) {
            ++num_players;
        }
    }
}


//
// D_QuitNetGame
// Called before quitting to leave a net game
// without hanging the other players
//

void D_QuitNetGame(void) {
    if(debugfile) {
        fclose(debugfile);
    }

#ifdef FEATURE_MULTIPLAYER
    NET_SV_Shutdown();
    NET_CL_Disconnect();
#endif

}

//
// PlayersInGame
// Returns true if there are currently any players in the game.
//

dboolean PlayersInGame(void) {
    int i;

    for(i = 0; i < MAXPLAYERS; ++i) {
        if(playeringame[i]) {
            return true;
        }
    }

    return false;
}

//
// GetLowTic
//

int GetLowTic(void) {
    int i;
    int lowtic;

    if(net_client_connected) {
        lowtic = INT_MAX;

        for(i = 0; i < MAXPLAYERS; ++i) {
            if(playeringame[i]) {
                if(nettics[i] < lowtic) {
                    lowtic = nettics[i];
                }
            }
        }
    }
    else {
        lowtic = maketic;
    }

    return lowtic;
}

