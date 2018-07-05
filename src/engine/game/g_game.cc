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
//
// DESCRIPTION:  none
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdarg.h>

#include "doomdef.h"
#include "doomstat.h"
#include "z_zone.h"
#include "f_finale.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_cheat.h"
#include "m_random.h"
#include "i_system.h"
#include "p_setup.h"
#include "p_saveg.h"
#include "p_tick.h"
#include "d_main.h"
#include "wi_stuff.h"
#include "st_stuff.h"
#include "am_map.h"
#include "p_local.h"
#include "s_sound.h"
#include "d_englsh.h"
#include "sounds.h"
#include "tables.h"
#include "info.h"
#include "r_local.h"
#include "r_wipe.h"
#include "con_console.h"
#include "g_local.h"
#include "m_password.h"
#include "i_video.h"
#include "g_demo.h"

#define DCLICK_TIME     20

void        G_PlayerReborn(int player);
void        G_InitNew(skill_t skill, int map);
void        G_DoReborn(int playernum);
void        G_DoLoadGame(void);
void        G_SetFastParms(int fast_pending);


gameaction_t    gameaction      = 0;
gamestate_t     gamestate       = 0;
skill_t         gameskill       = 0;
dboolean        respawnmonsters = false;
dboolean        respawnspecials = false;
int             gamemap         = 0;
int             nextmap         = 0;
dboolean        paused          = false;
dboolean        sendpause       = false;    // send a pause event next tic
dboolean        sendsave        = false;    // send a save event next tic
dboolean        usergame        = false;    // ok to save / end game
int             starttime       = 0;        // for comparative timing purposes
int             deathmatch      = false;    // only if started as net death
dboolean        netcheat        = false;
dboolean        netkill         = false;
dboolean        netgame         = false;    // only true if packets are broadcast
int             basetic         = 0;
int             gametic         = 0;

dboolean        playeringame[MAXPLAYERS];
player_t        players[MAXPLAYERS];

int             consoleplayer;              // player taking events and displaying
int             displayplayer;              // view being displayed

static dboolean savenow         = false;
static int      savegameflags   = 0;
static int      savecompatflags = 0;

// for intermission
int             totalkills, totalitems, totalsecret;
dboolean        precache        = true;     // if true, load all graphics at start

byte            consistency[MAXPLAYERS][BACKUPTICS];

#define MAXPLMOVE       (forwardmove[1])
#define TURBOTHRESHOLD  0x32

fixed_t     forwardmove[2]  =   { 0x1c, 0x2c };
fixed_t     sidemove[2]     =   { 0x1c, 0x2c };
fixed_t     angleturn[20]   =   { 0x32, 0x32, 0x53, 0x53, 0x64, 0x74, 0x85, 0x96, 0x96, 0xA6,
                                  0x85, 0x85, 0x96, 0xA6, 0xA6, 0xC8, 0xC8, 0xD8, 0xD8, 0xE9
                                };

#define SLOWTURNTICS    10

int         turnheld;                       // for accelerative turning
int         lookheld;

int         savegameslot;
char        savedescription[32];

playercontrols_t    Controls;

#define BODYQUESIZE 32

mobj_t*     bodyque[BODYQUESIZE];
int         bodyqueslot;

byte forcecollision = 0;
byte forcejump = 0;
byte forcefreelook = 0;

BoolCvar sv_nomonsters("sv_NoMonsters", "Disable monsters", false, Cvar::net_server);
BoolCvar sv_fastmonsters("sv_FastMonsters", "Fast monsters", false, Cvar::net_server);
BoolCvar sv_respawnitems("sv_RespawnItems", "Allow items to respawn", false, Cvar::net_server);
BoolCvar sv_respawn("sv_Respawn", "", false, Cvar::net_server);
IntCvar sv_skill("sv_Skill", "Skill level (0 - Easy, 4 - Nightmare)", 2, Cvar::net_server);

static void G_SetGameFlags();

extern BoolCvar v_mlook;
extern BoolCvar v_mlookinvert;
extern BoolCvar v_yaxismove;
extern BoolCvar p_autorun;
extern BoolCvar p_fdoubleclick;
extern BoolCvar p_sdoubleclick;
extern FloatCvar v_msensitivityx;
extern FloatCvar v_msensitivityy;

//
// G_CmdButton
//

static CMD(Button) {
    playercontrols_t *pc;
    int64 key;

    pc = &Controls;

    key = data & PCKF_COUNTMASK;

    if(data & PCKF_UP) {
        if((pc->key[key] & PCKF_COUNTMASK) > 0) {
            pc->key[key]--;
        }

        if(ButtonAction) {
            pc->key[key] &= ~PCKF_DOUBLEUSE;
        }
    }
    else {
        pc->key[key]++;

        if(ButtonAction) {
            pc->key[key] |= PCKF_DOUBLEUSE;
        }
    }
}

//
// G_CmdNextWeapon
//

static CMD(NextWeapon) {
    playercontrols_t *pc;

    pc = &Controls;
    pc->flags |= PCF_NEXTWEAPON;
}

//
// G_CmdPrevWeapon
//

static CMD(PrevWeapon) {
    playercontrols_t *pc;

    pc = &Controls;
    pc->flags |= PCF_PREVWEAPON;
}

//
// G_CmdWeapon
//

static CMD(Weapon) {
    playercontrols_t *pc;
    int id;

    if(!(param[0])) {
        return;
    }

    id = datoi(param[0]);

    if((id > NUMWEAPONS) || (id < 1)) {
        return;
    }

    pc = &Controls;
    pc->nextweapon = id - 1;
}

//
// G_CmdBind
//

static CMD(Bind) {
    if(!param[0]) {
        return;
    }

    if(!param[1]) {
        G_ShowBinding(param[0]);
        return;
    }

    G_BindActionByName(param[0], param[1]);
}

//
// G_CmdSeta
//

static CMD(Seta) {
    if(!param[0] || !param[1]) {
        return;
    }

    if (auto p = Cvar::find(param[0])) {
        p->set_string(param[1]);
    } else {
        I_Printf("Couldn't find property (cvar) %s\n", param[0]);
    }
}

//
// G_CmdAutorun
//

static CMD(Autorun) {
    if(gamestate != GS_LEVEL) {
        return;
    }

    if(!p_autorun) {
        p_autorun = true;
        players[consoleplayer].message = GGAUTORUNON;
    } else {
        p_autorun = false;
        players[consoleplayer].message = GGAUTORUNOFF;
    }
}

//
// G_CmdQuit
//

static CMD(Quit) {
    I_Quit();
}

//
// G_CmdExec
//

static CMD(Exec) {
    G_ExecuteFile(param[0]);
}

//
// G_CmdList
//

static CMD(List) {
    int cmds;

    CON_Printf(GREEN, "Available commands:\n");
    cmds = G_ListCommands();
    CON_Printf(GREEN, "(%d commands)\n", cmds);
}

//
// G_CmdCheat
//

static CMD(Cheat) {
    player_t *player;

    if(gamestate != GS_LEVEL) {
        return;
    }

    player = &players[consoleplayer];
    switch(data) {
    case 0:
        M_CheatGod(player, NULL);
        break;
    case 1:
        M_CheatClip(player, NULL);
        break;
    case 2:
        if(param[0] == NULL) {
            CON_Printf(GREEN, "Available give cheats:\n");
            CON_Printf(GREEN, "-------------------------\n");
            CON_Printf(AQUA, "all\n");
            CON_Printf(AQUA, "weapon\n");
            CON_Printf(AQUA, "artifact\n");
            CON_Printf(AQUA, "key\n");
            return;
        }

        if(!dstricmp(param[0], "all")) {
            M_CheatKfa(player, NULL);
        }
        else if(!dstricmp(param[0], "weapon")) {
            if(param[1] == NULL) {
                CON_Printf(GREEN, "Weapons:\n");
                CON_Printf(GREEN, "-------------------------\n");
                CON_Printf(AQUA, "1: Chainsaw\n");
                CON_Printf(AQUA, "2: Shotgun\n");
                CON_Printf(AQUA, "3: Super Shotgun\n");
                CON_Printf(AQUA, "4: Chaingun\n");
                CON_Printf(AQUA, "5: Rocket Launcher\n");
                CON_Printf(AQUA, "6: Plasma Rifle\n");
                CON_Printf(AQUA, "7: BFG 9000\n");
                CON_Printf(AQUA, "8: Demon Artifact\n");
                return;
            }

            if(dstrlen(param[1]) == 1) {
                M_CheatGiveWeapon(player, param[1]);
            }
        }
        else if(!dstricmp(param[0], "artifact")) {
            if(param[1] == NULL) {
                CON_Printf(GREEN, "Artifacts:\n");
                CON_Printf(GREEN, "-------------------------\n");
                CON_Printf(AQUA, "1: Red\n");
                CON_Printf(AQUA, "2: Aqua\n");
                CON_Printf(AQUA, "3: Violet\n");
                return;
            }

            if(dstrlen(param[1]) == 1) {
                M_CheatArtifacts(player, param[1]);
            }
        }
        else if(!dstricmp(param[0], "key")) {
            if(param[1] == NULL) {
                CON_Printf(GREEN, "Keys:\n");
                CON_Printf(GREEN, "-------------------------\n");
                CON_Printf(AQUA, "1: Blue Card\n");
                CON_Printf(AQUA, "2: Yellow Card\n");
                CON_Printf(AQUA, "3: Red Card\n");
                CON_Printf(AQUA, "4: Blue Skull\n");
                CON_Printf(AQUA, "5: Yellow Skull\n");
                CON_Printf(AQUA, "6: Red Skull\n");
                return;
            }

            if(dstrlen(param[1]) == 1) {
                M_CheatGiveKey(player, param[1]);
            }
        }
        break;
    case 3:
        M_CheatBoyISuck(player, NULL);
        break;
    case 4:
        if(amCheating) {
            amCheating = 0;
        }
        else if(!amCheating) {
            amCheating = 2;
        }
        break;
    }
}

//
// G_CmdPause
//

static CMD(Pause) {
    sendpause = true;
}

//
// G_CmdSpawnThing
//

static CMD(SpawnThing) {
    int id = 0;
    player_t *player;
    mobj_t *thing;
    fixed_t x, y, z;

    if(gamestate != GS_LEVEL) {
        return;
    }

    if(!param[0]) {
        return;
    }

    if(netgame) {
        return;
    }

    id = datoi(param[0]);
    if(id >= NUMMOBJTYPES || id < 0) {
        return;
    }

    player = &players[consoleplayer];
    x = player->mo->x + FixedMul(INT2F(64) + mobjinfo[id].radius, dcos(player->mo->angle));
    y = player->mo->y + FixedMul(INT2F(64) + mobjinfo[id].radius, dsin(player->mo->angle));
    z = player->mo->z;

    thing = P_SpawnMobj(x, y, z, id);

    if(thing->info->spawnstate == S_000) {
        P_RemoveMobj(thing);
        return;
    }

    thing->angle = player->mo->angle;
}

//
// G_CmdExitLevel
//

static CMD(ExitLevel) {
    if(gamestate != GS_LEVEL) {
        return;
    }

    if(demoplayback) {
        return;
    }

    if(!param[0]) {
        G_ExitLevel();
    }
    else {
        G_SecretExitLevel(datoi(param[0]));
    }
}

//
// G_CmdTriggerSpecial
//

static CMD(TriggerSpecial) {
    line_t junk;

    if(gamestate != GS_LEVEL) {
        return;
    }

    if(!param[0]) {
        return;
    }

    dmemset(&junk, 0, sizeof(line_t));
    junk.special = datoi(param[0]);
    junk.tag = datoi(param[1]);

    P_DoSpecialLine(players[consoleplayer].mo, &junk, 0);
}

//
// G_CmdPlayerCamera
//

static CMD(PlayerCamera) {
    player_t *player;

    if(gamestate != GS_LEVEL) {
        return;
    }

    player = &players[consoleplayer];

    switch(data) {
    case 0:
        P_SetStaticCamera(player);
        break;

    case 1:
        P_SetFollowCamera(player);
        break;
    }
}

//
// G_CmdEndDemo
//

static CMD(EndDemo) {
    endDemo = true;
}

//
// G_SaveDefaults
//

void G_SaveDefaults(void) {
    savegameflags = gameflags;
    savecompatflags = compatflags;
}

//
// G_ReloadDefaults
//

void G_ReloadDefaults(void) {
    rngseed += I_GetRandomTimeSeed() + gametic;
    gameflags = savegameflags;
    compatflags = savecompatflags;
}


//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer.
// If recording a demo, write it out
//

void G_BuildTiccmd(ticcmd_t* cmd) {
    int                 i;
    int                 speed;
    int                 forward;
    int                 side;
    playercontrols_t    *pc;

    pc = &Controls;
    dmemset(cmd, 0, sizeof(ticcmd_t));

    cmd->consistency = consistency[consoleplayer][maketic % BACKUPTICS];

    if(pc->key[PCKEY_RUN]) {
        speed = 1;
    }
    else {
        speed=0;
    }

    if(p_autorun) {
        speed = !speed;
    }

    forward = side = 0;

    // use two stage accelerative turning
    // on the keyboard and joystick
    if(pc->key[PCKEY_LEFT] || pc->key[PCKEY_RIGHT]) {
        turnheld += ticdup;
    }
    else {
        turnheld = 0;
    }

    if(turnheld >= SLOWTURNTICS) {
        turnheld = SLOWTURNTICS-1;
    }

    if(pc->key[PCKEY_LOOKUP] || pc->key[PCKEY_LOOKDOWN]) {
        lookheld += ticdup;
    }
    else {
        lookheld = 0;
    }

    if(lookheld >= SLOWTURNTICS) {
        lookheld = SLOWTURNTICS-1;
    }

    if(pc->key[PCKEY_STRAFE]) {
        if(pc->key[PCKEY_RIGHT]) {
            side += sidemove[speed];
        }
        if(pc->key[PCKEY_LEFT]) {
            side -= sidemove[speed];
        }
        side += sidemove[1] * pc->mousex * 2;
    }
    else {
        if(pc->key[PCKEY_RIGHT]) {
            cmd->angleturn -= angleturn[turnheld + (speed ? SLOWTURNTICS : 0)] << 2;
        }

        if(pc->key[PCKEY_LEFT]) {
            cmd->angleturn += angleturn[turnheld + (speed ? SLOWTURNTICS : 0)] << 2;
        }

        if(pc->key[PCKEY_LOOKUP]) {
            cmd->pitch += angleturn[lookheld + (speed ? SLOWTURNTICS : 0)] << 2;
        }

        if(pc->key[PCKEY_LOOKDOWN]) {
            cmd->pitch -= angleturn[lookheld + (speed ? SLOWTURNTICS : 0)] << 2;
        }

        cmd->angleturn -= pc->mousex * 0x8;

        if(forcefreelook != 2) {
            if(v_mlook || forcefreelook) {
                cmd->pitch -= v_mlookinvert ? pc->mousey * 0x8 : -(pc->mousey * 0x8);
            }
        }
    }

    if(v_yaxismove) {
        forward += pc->mousey;
    }

    if(pc->key[PCKEY_CENTER]) {
        cmd->buttons2 |= BT2_CENTER;
    }

    if(pc->key[PCKEY_FORWARD]) {
        forward += forwardmove[speed];
    }

    //
    // forward/side movement with joystick
    //
    if(pc->flags & PCF_GAMEPAD) {
        forward += pc->joyy;
        side += pc->joyx;
    }

    if(pc->key[PCKEY_BACK]) {
        forward -= forwardmove[speed];
    }

    if(pc->key[PCKEY_STRAFERIGHT]) {
        side += sidemove[speed];
    }

    if(pc->key[PCKEY_STRAFELEFT]) {
        side -= sidemove[speed];
    }

    pc->mousex = pc->mousey = 0;
    pc->joyx = pc->joyy = 0;

    cmd->chatchar = ST_DequeueChatChar();

    if(pc->key[PCKEY_ATTACK]) {
        cmd->buttons |= BT_ATTACK;
    }

    if(pc->key[PCKEY_USE]) {
        cmd->buttons |= BT_USE;
        // clear double clicks if hit use button
        pc->flags &= ~(PCF_FDCLICK2|PCF_SDCLICK2);
    }

    if(forcejump != 2) {
        if(gameflags & GF_ALLOWJUMP || forcejump) {
            if(pc->key[PCKEY_JUMP]) {
                cmd->buttons2 |= BT2_JUMP;
            }
        }
    }

    if(pc->flags & PCF_NEXTWEAPON) {
        cmd->buttons |= BT_CHANGE;
        cmd->buttons2 |= BT2_NEXTWEAP;
        pc->flags &= ~PCF_NEXTWEAPON;
    }
    else if(pc->flags & PCF_PREVWEAPON) {
        cmd->buttons |= BT_CHANGE;
        cmd->buttons2 |= BT2_PREVWEAP;
        pc->flags &= ~PCF_PREVWEAPON;
    }
    else if(pc->nextweapon != wp_nochange) {
        cmd->buttons |= BT_CHANGE;
        cmd->buttons |= pc->nextweapon << BT_WEAPONSHIFT;
        pc->nextweapon = wp_nochange;
    }

    //doubleclick use
    i=pc->flags & PCF_FDCLICK;
    if(pc->key[PCKEY_FORWARD] & PCKF_DOUBLEUSE) {
        i ^= PCF_FDCLICK;
    }

    if(i) {
        pc->flags ^= PCF_FDCLICK;
        if(pc->key[PCKEY_FORWARD] & PCKF_DOUBLEUSE) {
            if(pc->flags & PCF_FDCLICK2) {
                if(p_fdoubleclick) {
                    cmd->buttons |= BT_USE;
                }

                pc->flags &= ~PCF_FDCLICK2;
            }
            else {
                pc->flags |= PCF_FDCLICK2;
            }
        }
        pc->fdclicktime = 0;
    }
    else if(pc->fdclicktime >= 0) {
        pc->fdclicktime += ticdup;
        if(pc->fdclicktime > DCLICK_TIME) {
            pc->flags &= ~PCF_FDCLICK2;
            pc->fdclicktime = -1;
        }
    }

    i = pc->flags & PCF_SDCLICK;
    if(pc->key[PCKEY_STRAFE] & PCKF_DOUBLEUSE) {
        i ^= PCF_SDCLICK;
    }

    if(i) {
        pc->flags ^= PCF_SDCLICK;
        if(pc->key[PCKEY_STRAFE] & PCKF_DOUBLEUSE) {
            if(pc->flags & PCF_SDCLICK2) {
                if(p_sdoubleclick) {
                    cmd->buttons |= BT_USE;
                }

                pc->flags &= ~PCF_SDCLICK2;
            }
            else {
                pc->flags |= PCF_SDCLICK2;
            }
        }
        pc->sdclicktime = 0;
    }
    else if(pc->sdclicktime >= 0) {
        pc->sdclicktime += ticdup;
        if(pc->sdclicktime > DCLICK_TIME) {
            pc->flags &= ~PCF_SDCLICK2;
            pc->sdclicktime = -1;
        }
    }


    if(forward > MAXPLMOVE) {
        forward = MAXPLMOVE;
    }
    else if(forward < -MAXPLMOVE) {
        forward = -MAXPLMOVE;
    }

    if(side > MAXPLMOVE) {
        side = MAXPLMOVE;
    }
    else if(side < -MAXPLMOVE) {
        side = -MAXPLMOVE;
    }

    cmd->forwardmove += forward;
    cmd->sidemove += side;

    // special buttons
    if(sendpause) {
        sendpause = false;
        cmd->buttons = BT_SPECIAL | BTS_PAUSE;
    }

    if(sendsave) {
        sendsave = false;
        cmd->buttons = BT_SPECIAL | BTS_SAVEGAME | (savegameslot << BTS_SAVESHIFT);
    }
}

//
// G_DoCmdMouseMove
//

void G_DoCmdMouseMove(int x, int y) {
    playercontrols_t *pc;

    pc = &Controls;
    pc->mousex += static_cast<int>((x * *v_msensitivityx) / 128);
    pc->mousey += static_cast<int>((y * *v_msensitivityy) / 128);
}


//
// G_ClearInput
//

void G_ClearInput(void) {
    int                 i;
    playercontrols_t    *pc;

    pc = &Controls;
    pc->flags = 0;
    pc->nextweapon = wp_nochange;
    for(i = 0; i < NUM_PCKEYS; i++) {
        pc->key[i] = 0;
    }
}

//
// G_SetGameFlags
//

static void G_SetGameFlags(void) {
    gameflags = 0;
    compatflags = 0;

    if (sv_lockmonsters)
        gameflags |= GF_LOCKMONSTERS;

    if (sv_allowcheats)
        gameflags |= GF_ALLOWCHEATS;

    if (sv_friendlyfire)
        gameflags |= GF_FRIENDLYFIRE;

    if (sv_keepitems)
        gameflags |= GF_KEEPITEMS;

    if (p_allowjump)
         gameflags |= GF_ALLOWJUMP;

    if (p_autoaim)
         gameflags |= GF_ALLOWAUTOAIM;

    if (compat_collision)
        compatflags |= COMPATF_COLLISION;

    if (compat_mobjpass)
        compatflags |= COMPATF_MOBJPASS;

    if (compat_limitpain)
        compatflags |= COMPATF_LIMITPAIN;

    if (compat_grabitems)
        compatflags |= COMPATF_REACHITEMS;
}

//
// G_DoLoadLevel
//

void G_DoLoadLevel(void) {
    int i;
    mapdef_t* map;

    for(i = 0; i < MAXPLAYERS; i++) {
        if(playeringame[i] && players[i].playerstate == PST_DEAD) {
            players[i].playerstate = PST_REBORN;
        }

        dmemset(players[i].frags, 0, sizeof(players[i].frags));
    }

    basetic = gametic;

    // update settings from server cvar
    if(!netgame) {
        gameskill   = *sv_skill;
        respawnparm = *sv_respawn;
        respawnitem = *sv_respawnitems;
        fastparm    = *sv_fastmonsters;
        nomonsters  = *sv_nomonsters;
    }

    map = P_GetMapInfo(gamemap);

    if(map == NULL) {
        // boot out to main menu
        gameaction = ga_title;
        return;
    }

    forcecollision  = map->oldcollision;
    forcejump       = map->allowjump;
    forcefreelook   = map->allowfreelook;

    // This was quite messy with SPECIAL and commented parts.
    // Supposedly hacks to make the latest edition work.
    // It might not work properly.

    G_SetFastParms(fastparm || gameskill == sk_nightmare);  // killough 4/10/98

    if(gameskill == sk_nightmare || respawnparm) {
        respawnmonsters = true;
    }
    else {
        respawnmonsters = false;
    }

    if(respawnitem) {
        respawnspecials = true;
    }
    else {
        respawnspecials = false;
    }

    P_SetupLevel(gamemap, 0, gameskill);
    displayplayer = consoleplayer;        // view the guy you are playing
    starttime = I_GetTime();
    gameaction = ga_nothing;

    // clear cmd building stuff
    G_ClearInput();
    sendpause = sendsave = paused = false;
}


//
// G_Responder
// Get info needed to make ticcmd_ts for the players.
//

dboolean G_Responder(event_t* ev) {
    // Handle level specific ticcmds
    if(gamestate == GS_LEVEL) {
        // allow spy mode changes even during the demo
        if(ev->type == ev_keydown
                && ev->data1 == KEY_F12 && (singledemo || !deathmatch)) {
            // spy mode
            do {
                displayplayer++;
                if(displayplayer == MAXPLAYERS) {
                    displayplayer = 0;
                }

            }
            while(!playeringame[displayplayer] && displayplayer != consoleplayer);

            return true;
        }

        if(demoplayback && gameaction == ga_nothing) {
            if(ev->type == ev_keydown ||
                    ev->type == ev_gamepad) {
                G_CheckDemoStatus();
                gameaction = ga_warpquick;
                return true;
            }
            else {
                return false;
            }
        }

        if(ST_Responder(ev)) {
            return true;    // status window ate it
        }

        if(AM_Responder(ev)) {
            return true;    // automap ate it
        }
    }

    // Handle screen specific ticcmds
    if(gamestate == GS_SKIPPABLE) {
        if(gameaction == ga_nothing) {
            if(ev->type == ev_keydown ||
                    (ev->type == ev_mouse && ev->data1) ||
                    ev->type == ev_gamepad) {
                gameaction = ga_title;
                return true;
            }
            return false;
        }
    }

    if((ev->type == ev_keydown) && (ev->data1 == KEY_PAUSE)) {
        sendpause = true;
        return true;
    }

    if(G_ActionResponder(ev)) {
        return true;
    }

    return false;
}

//
// G_Ticker
// Make ticcmd_ts for the players.
//

void G_Ticker(void) {
    int         i;
    int         buf;
    ticcmd_t*   cmd;

    G_ActionTicker();
    CON_Ticker();

    if(savenow) {
        G_DoSaveGame();
        savenow = false;
    }

    if(gameaction == ga_screenshot) {
        M_ScreenShot();
        gameaction = ga_nothing;
    }

    if(paused & 2 || (!demoplayback && menuactive && !netgame)) {
        basetic++;    // For tracers and RNG -- we must maintain sync
    }
    else {
        // get commands, check consistency,
        // and build new consistency check
        buf = (gametic / ticdup) % BACKUPTICS;

        for(i = 0; i < MAXPLAYERS; i++) {
            if(playeringame[i]) {
                cmd = &players[i].cmd;

                dmemcpy(cmd, &netcmds[i][buf], sizeof(ticcmd_t));

                //
                // 20120404 villsa - make sure gameaction isn't set to anything before
                // reading a demo lump
                //
                if(demoplayback && gameaction == ga_nothing) {
                    G_ReadDemoTiccmd(cmd);
                }

                if(demorecording) {
                    G_WriteDemoTiccmd(cmd);

                    if(endDemo == true) {
                        G_CheckDemoStatus();
                    }
                }

                if(netgame && !netdemo && !(gametic % ticdup)) {
                    if(gametic > BACKUPTICS
                            && consistency[i][buf] != cmd->consistency) {
                        I_Error("consistency failure (%i should be %i)",
                                cmd->consistency, consistency[i][buf], consoleplayer);
                    }
                    if(players[i].mo) {
                        consistency[i][buf] = players[i].mo->x;
                    }
                    else {
                        consistency[i][buf] = 0;
                    }
                }
            }
        }
    }

    // check for special buttons
    for(i = 0; i < MAXPLAYERS; i++) {
        if(playeringame[i]) {
            if(players[i].cmd.buttons & BT_SPECIAL) {
                /*villsa - fixed crash when player restarts level after dying
                    Changed switch statments to if statments*/
                if((players[i].cmd.buttons & BT_SPECIALMASK) == BTS_PAUSE) {
                    paused ^= 1;
                    if(paused) {
                        S_PauseSound();
                    }
                    else {
                        S_ResumeSound();
                    }
                }

                if((players[i].cmd.buttons & BT_SPECIALMASK) == BTS_SAVEGAME) {
                    if(!savedescription[0]) {
                        dstrcpy(savedescription, "NET GAME");
                    }
                    savegameslot =
                        (players[i].cmd.buttons & BTS_SAVEMASK)>>BTS_SAVESHIFT;
                    savenow = true;
                }
            }
        }
    }
}

//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Mobj
//

//
// G_PlayerFinishLevel
// Can when a player completes a level.
//

void G_PlayerFinishLevel(int player) {
    player_t*   p;

    p = &players[player];

    dmemset(p->powers, 0, sizeof(p->powers));
    dmemset(p->cards, 0, sizeof(p->cards));
    p->mo->flags &= ~MF_SHADOW;     // cancel invisibility
    p->damagecount = 0;         // no palette changes
    p->bonuscount = 0;
    p->bfgcount = 0;

    P_ClearUserCamera(p);
}

//
// G_PlayerReborn
// Called after a player dies
// almost everything is cleared and initialized
//

void G_PlayerReborn(int player) {
    player_t    *p;
    int         i;
    int         frags[MAXPLAYERS];
    int         killcount;
    int         itemcount;
    int         secretcount;
    dboolean    cards[NUMCARDS];
    dboolean    wpns[NUMWEAPONS];
    int         pammo[NUMAMMO];
    int         pmaxammo[NUMAMMO];
    int         artifacts;
    dboolean    backpack;

    dmemcpy(frags, players[player].frags, sizeof(frags));
    dmemcpy(cards, players[player].cards, sizeof(dboolean)*NUMCARDS);
    dmemcpy(wpns, players[player].weaponowned, sizeof(dboolean)*NUMWEAPONS);
    dmemcpy(pammo, players[player].ammo, sizeof(int)*NUMAMMO);
    dmemcpy(pmaxammo, players[player].maxammo, sizeof(int)*NUMAMMO);

    backpack = players[player].backpack;
    artifacts = players[player].artifacts;
    killcount = players[player].killcount;
    itemcount = players[player].itemcount;
    secretcount = players[player].secretcount;

    quakeviewx = 0;
    quakeviewy = 0;
    infraredFactor = 0;
    R_RefreshBrightness();

    p = &players[player];
    dmemset(p, 0, sizeof(*p));

    dmemcpy(players[player].frags, frags, sizeof(players[player].frags));
    players[player].killcount = killcount;
    players[player].itemcount = itemcount;
    players[player].secretcount = secretcount;

    p->usedown = p->attackdown = p->jumpdown = true;  // don't do anything immediately
    p->playerstate = PST_LIVE;
    p->health = MAXHEALTH;
    p->readyweapon = p->pendingweapon = wp_pistol;
    p->weaponowned[wp_fist] = true;
    p->weaponowned[wp_pistol] = true;
    p->ammo[am_clip] = 50;
    p->recoilpitch = 0;

    for(i = 0; i < NUMAMMO; i++) {
        p->maxammo[i] = maxammo[i];
    }

    if(netgame) {
        for(i = 0; i < NUMCARDS; i++) {
            players[player].cards[i] = cards[i];
        }

        if(gameflags & GF_KEEPITEMS) {
            p->artifacts = artifacts;
            p->backpack = backpack;

            for(i = 0; i < NUMAMMO; i++) {
                p->ammo[i] = pammo[i];
                p->maxammo[i] = pmaxammo[i];
            }

            for(i = 0; i < NUMWEAPONS; i++) {
                p->weaponowned[i] = wpns[i];
            }
        }
    }
}

//
// G_CheckSpot
// Returns false if the player cannot be respawned
// at the given mapthing_t spot
// because something is occupying it
//

dboolean G_CheckSpot(int playernum, mapthing_t* mthing) {
    fixed_t         x;
    fixed_t         y;
    subsector_t*    ss;
    angle_t         an;
    mobj_t*         mo;
    int             i;

    if(!players[playernum].mo) {
        // first spawn of level, before corpses
        for(i = 0; i < playernum; i++) {
            if((players[i].mo->x == INT2F(mthing->x)) && (players[i].mo->y == INT2F(mthing->y))) {
                return false;
            }
        }
        return true;
    }

    x = INT2F(mthing->x);
    y = INT2F(mthing->y);

    if(!P_CheckPosition(players[playernum].mo, x, y)) {
        return false;
    }

    // flush an old corpse if needed
    if(bodyqueslot >= BODYQUESIZE) {
        P_RemoveMobj(bodyque[bodyqueslot % BODYQUESIZE]);
    }

    bodyque[bodyqueslot%BODYQUESIZE] = players[playernum].mo;
    bodyqueslot++;

    // spawn a teleport fog
    ss = R_PointInSubsector(x, y);

    // 20120402 villsa - force angle_t typecast to avoid issues on 64-bit machines
    an = ANG45 * (angle_t)(mthing->angle / 45);

    mo = P_SpawnMobj(
             x + 20*dcos(an),
             y + 20*dsin(an),
             ss->sector->floorheight,
             MT_TELEPORTFOG
         );

    if(players[playernum].viewz != 1) {
        S_StartSound(mo, sfx_telept);    // don't start sound on first frame
    }

    return true;
}


//
// G_DeathMatchSpawnPlayer
// Spawns a player at one of the random death match spots
// called at level load and each death
//

void G_DeathMatchSpawnPlayer(int playernum) {
    int i, j;
    int selections;

    selections = deathmatch_p - deathmatchstarts;
    if(selections < 4) {
        I_Error("G_DeathMatchSpawnPlayer: Only %i deathmatch spots, 4 required", selections);
    }

    for(j = 0; j < 20; j++) {
        i = P_Random(pr_dmspawn) % selections;
        if(G_CheckSpot(playernum, &deathmatchstarts[i])) {
            deathmatchstarts[i].type = playernum+1;
            P_SpawnPlayer(&deathmatchstarts[i]);
            return;
        }
    }

    // no good spot, so the player will probably get stuck
    P_SpawnPlayer(&playerstarts[playernum]);
}

//
// G_DoReborn
//

void G_DoReborn(int playernum) {
    if(!netgame) {
        gameaction = ga_loadlevel;    // reload the level from scratch
    }
    else {   // respawn at the start
        int i;

        // first dissasociate the corpse
        if(players[playernum].mo == NULL) {
            I_Error("G_DoReborn: Player start #%i not found!", playernum+1);
        }

        players[playernum].mo->player = NULL;

        // spawn at random spot if in death match
        if(deathmatch) {
            G_DeathMatchSpawnPlayer(playernum);
            return;
        }

        if(G_CheckSpot(playernum, &playerstarts[playernum])) {
            P_SpawnPlayer(&playerstarts[playernum]);
            return;
        }

        // try to spawn at one of the other players spots
        for(i = 0; i < MAXPLAYERS; i++) {
            if(G_CheckSpot(playernum, &playerstarts[i])) {
                playerstarts[i].type = playernum+1;    // fake as other player
                P_SpawnPlayer(&playerstarts[i]);
                playerstarts[i].type = i+1;            // restore
                return;
            }
            // he's going to be inside something.  Too bad.
        }
        P_SpawnPlayer(&playerstarts[playernum]);
    }
}

//
// G_ScreenShot
//

void G_ScreenShot(void) {
    gameaction = ga_screenshot;
}

//
// G_CompleteLevel
//

void G_CompleteLevel(void) {
    mapdef_t* map;
    clusterdef_t* cluster;

    map = P_GetMapInfo(gamemap);

    if(!map->nointermission) {
        gameaction = ga_completed;
    }
    else {
        gameaction = ga_victory;
    }

    cluster = P_GetCluster(nextmap);

    if(cluster && cluster->nointermission) {
        gameaction = ga_victory;
    }
}

//
// G_ExitLevel
//

void G_ExitLevel(void) {
    line_t junk;
    mapdef_t* map;

    map = P_GetMapInfo(gamemap);

    dmemset(&junk, 0, sizeof(line_t));
    junk.tag = map->exitdelay;

    P_SpawnDelayTimer(&junk, G_CompleteLevel);

    nextmap = gamemap + 1;
}

//
// G_SecretExitLevel
//

void G_SecretExitLevel(int map) {
    line_t junk;
    mapdef_t* mapdef;

    mapdef = P_GetMapInfo(gamemap);

    dmemset(&junk, 0, sizeof(line_t));
    junk.tag = mapdef->exitdelay;

    P_SpawnDelayTimer(&junk, G_CompleteLevel);

    nextmap = map;
}

//
// G_RunTitleMap
//

void G_RunTitleMap(void) {
    // villsa 12092013 - abort if map doesn't exist in mapfino
    if(P_GetMapInfo(33) == NULL) {
        return;
    }

    demobuffer = (byte*) Z_Calloc(0x16000, PU_STATIC, NULL);
    demo_p = demobuffer;
    demobuffer[0x16000-1] = DEMOMARKER;

    G_InitNew(sk_medium, 33);

    precache = true;
    usergame = false;
    demoplayback = true;
    iwadDemo = true;

    rngseed = 0;

    G_DoLoadLevel();
    D_MiniLoop(P_Start, P_Stop, P_Drawer, P_Ticker);
}

//
// G_RunGame
// The game should already have been initialized or loaded
//

void G_RunGame(void) {
    int next = 0;

    if(!demorecording && !demoplayback) {
        G_ReloadDefaults();
        G_InitNew(startskill, startmap);
    }

    while(gameaction != ga_title) {
        if(gameaction == ga_loadgame) {
            G_DoLoadGame();
        }
        else {
            G_DoLoadLevel();

            if(gameaction == ga_title) {
                break;
            }
        }

        next = D_MiniLoop(P_Start, P_Stop, P_Drawer, P_Ticker);

        if(next == ga_loadlevel) {
            continue;    // restart level from scratch
        }

        if(next == ga_warplevel || next == ga_warpquick) {
            continue;    // skip intermission
        }

        if(next == ga_title) {
            return;    // exit game and return to title screen
        }

        if(next == ga_completed) {
            next = D_MiniLoop(WI_Start, WI_Stop, WI_Drawer, WI_Ticker);
        }

        if(next == ga_victory) {
            next = D_MiniLoop(IN_Start, IN_Stop, IN_Drawer, IN_Ticker);

            if(next == ga_finale) {
                D_MiniLoop(F_Start, F_Stop, F_Drawer, F_Ticker);
            }
        }

        gamemap = nextmap;
    }
}

char savename[256];

//
// G_LoadGame
//

void G_LoadGame(const char* name) {
    dstrcpy(savename, name);
    gameaction = ga_loadgame;
}

#define VERSIONSIZE     16

//
// G_DoLoadGame
//

void G_DoLoadGame(void) {
    CON_DPrintf("--------Loading game--------\n");

    if(!P_ReadSaveGame(savename)) {
        gameaction = ga_nothing;
        players[consoleplayer].message = "couldn't load game!";
        return;
    }
}


//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string
//

void G_SaveGame(int slot, const char* description) {
    savegameslot = slot;
    dstrcpy(savedescription, description);
    sendsave = true;
}

//
// G_DoSaveGame
//

void G_DoSaveGame(void) {
    CON_DPrintf("--------Saving game--------\n");

    if(!P_WriteSaveGame(savedescription, savegameslot)) {
        players[consoleplayer].message = "couldn't save game!";
        return;
    }

    savedescription[0] = 0;
    players[consoleplayer].message = GGSAVED;
}


//
// G_DeferedInitNew
// Can be called by the startup code or the menu task,
// consoleplayer, displayplayer, playeringame[] should be set.
//

void G_DeferedInitNew(skill_t skill, int map) {
    startskill = skill;
    startmap = map;
    nextmap = map;
    gameaction = ga_newgame;
}

//
// G_Init
//

void G_Init(void) {
    G_ReloadDefaults();
    G_InitActions();

    dmemset(playeringame, 0, sizeof(playeringame));
    G_ClearInput();

    G_AddCommand("+fire", CMD_Button, PCKEY_ATTACK);
    G_AddCommand("-fire", CMD_Button, PCKEY_ATTACK|PCKF_UP);
    G_AddCommand("+strafe", CMD_Button, PCKEY_STRAFE);
    G_AddCommand("-strafe", CMD_Button, PCKEY_STRAFE|PCKF_UP);
    G_AddCommand("+use", CMD_Button, PCKEY_USE);
    G_AddCommand("-use", CMD_Button, PCKEY_USE|PCKF_UP);
    G_AddCommand("+run", CMD_Button, PCKEY_RUN);
    G_AddCommand("-run", CMD_Button, PCKEY_RUN|PCKF_UP);
    G_AddCommand("+jump", CMD_Button, PCKEY_JUMP);
    G_AddCommand("-jump", CMD_Button, PCKEY_JUMP|PCKF_UP);
    G_AddCommand("weapon", CMD_Weapon, 0);
    G_AddCommand("nextweap", CMD_NextWeapon, 0);
    G_AddCommand("prevweap", CMD_PrevWeapon, 0);
    G_AddCommand("+forward", CMD_Button, PCKEY_FORWARD);
    G_AddCommand("-forward", CMD_Button, PCKEY_FORWARD|PCKF_UP);
    G_AddCommand("+back", CMD_Button, PCKEY_BACK);
    G_AddCommand("-back", CMD_Button, PCKEY_BACK|PCKF_UP);
    G_AddCommand("+left", CMD_Button, PCKEY_LEFT);
    G_AddCommand("-left", CMD_Button, PCKEY_LEFT|PCKF_UP);
    G_AddCommand("+right", CMD_Button, PCKEY_RIGHT);
    G_AddCommand("-right", CMD_Button, PCKEY_RIGHT|PCKF_UP);
    G_AddCommand("+lookup", CMD_Button, PCKEY_LOOKUP);
    G_AddCommand("-lookup", CMD_Button, PCKEY_LOOKUP|PCKF_UP);
    G_AddCommand("+lookdown", CMD_Button, PCKEY_LOOKDOWN);
    G_AddCommand("-lookdown", CMD_Button, PCKEY_LOOKDOWN|PCKF_UP);
    G_AddCommand("+center",  CMD_Button, PCKEY_CENTER);
    G_AddCommand("-center",  CMD_Button, PCKEY_CENTER|PCKF_UP);
    G_AddCommand("autorun",  CMD_Autorun, 0);
    G_AddCommand("+strafeleft", CMD_Button, PCKEY_STRAFELEFT);
    G_AddCommand("-strafeleft", CMD_Button, PCKEY_STRAFELEFT|PCKF_UP);
    G_AddCommand("+straferight", CMD_Button, PCKEY_STRAFERIGHT);
    G_AddCommand("-straferight", CMD_Button, PCKEY_STRAFERIGHT|PCKF_UP);
    G_AddCommand("bind", CMD_Bind, 0);
    G_AddCommand("seta", CMD_Seta, 0);
    G_AddCommand("quit", CMD_Quit, 0);
    G_AddCommand("exec", CMD_Exec, 0);
    G_AddCommand("listcmd", CMD_List, 0);
    G_AddCommand("god",  CMD_Cheat, 0);
    G_AddCommand("noclip", CMD_Cheat, 1);
    G_AddCommand("give", CMD_Cheat, 2);
    G_AddCommand("killall", CMD_Cheat, 3);
    G_AddCommand("mapall", CMD_Cheat, 4);
    G_AddCommand("pause", CMD_Pause, 0);
    G_AddCommand("spawnthing", CMD_SpawnThing, 0);
    G_AddCommand("exitlevel", CMD_ExitLevel, 0);
    G_AddCommand("trigger", CMD_TriggerSpecial, 0);
    G_AddCommand("setcamerastatic", CMD_PlayerCamera, 0);
    G_AddCommand("setcamerachase", CMD_PlayerCamera, 1);
    G_AddCommand("enddemo", CMD_EndDemo, 0);
    AM_RegisterCommands();
}

//
// G_SetFastParms
// killough 4/10/98: New function to fix bug which caused Doom
// lockups when idclev was used in conjunction with -fast.

void G_SetFastParms(int fast_pending) {
    static int fast = 0;            // remembers fast state
    int i;
    if(fast != fast_pending) {
        /* only change if necessary */
        if((fast = fast_pending)) {
            for(i = S_044; i <= S_058; i++) {
                if(states[i].tics != 1) { // killough 4/10/98
                    states[i].tics >>= 1;    // don't change 1->0 since it causes cycles
                }
            }
            mobjinfo[MT_PROJ_BRUISER1].speed = 20*FRACUNIT;
            mobjinfo[MT_PROJ_HEAD].speed = 30*FRACUNIT;
            mobjinfo[MT_PROJ_IMP2].speed = 35*FRACUNIT;
            mobjinfo[MT_PROJ_IMP1].speed = 20*FRACUNIT;
        }
        else {
            for(i = S_044; i <= S_058; i++) {
                states[i].tics <<= 1;
            }

            mobjinfo[MT_PROJ_BRUISER1].speed = 15*FRACUNIT;
            mobjinfo[MT_PROJ_HEAD].speed = 20*FRACUNIT;
            mobjinfo[MT_PROJ_IMP2].speed = 20*FRACUNIT;
            mobjinfo[MT_PROJ_IMP1].speed = 10*FRACUNIT;
        }
    }
}

//
// G_InitNew
//

void G_InitNew(skill_t skill, int map) {
    int i;

    if(!netgame) {
        netdemo = false;
        netgame = false;
        deathmatch = false;
        playeringame[1] = playeringame[2] = playeringame[3] = 0;
        playeringame[0]=true;
        consoleplayer = 0;
    }

    if(paused) {
        paused = false;
    }

    if(skill > sk_nightmare) {
        skill = sk_nightmare;
    }

    // force players to be initialized upon first level load
    for(i = 0; i < MAXPLAYERS; i++) {
        players[i].playerstate = PST_REBORN;
    }

    usergame            = true;                // will be set false if a demo
    paused              = false;
    demoplayback        = false;
    automapactive       = false;
    gamemap             = map;
    gameskill           = skill;

    G_SetGameFlags();

    // [d64] For some reason this is added here
    M_ClearRandom();

    sv_skill = skill;
}
