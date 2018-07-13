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
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of
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
// DESCRIPTION:
//      Doom3D action parsing system
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include "g_local.h"
#include "m_keys.h"
#include "i_system.h"
#include "i_video.h"
#include "doomstat.h"
#include "con_console.h"
#include "z_zone.h"
#include <core/cvar.hh>

//do controls menu length properly
//if list of actions for menu>=MAX_MENUACTION_LENGTH then won't display any more
#define MAX_MENUACTION_LENGTH 25

#define MAX_CURRENTACTIONS    16

typedef struct action_s action_t;

struct action_s {
    char            *name;
    actionproc_t    proc;
    action_t        *children[2];
    action_t        *parent;
    int64           data;
};

static action_t    *Actions=NULL;

typedef struct alist_s alist_t;

struct alist_s {
    char        *buff;
    char        *cmd;
    alist_t     *next;
    int         refcount;
    //should allocate to required size?
    char        *param[MAX_ACTIONPARAM+1];//NULL terminated list
};

void G_RunAlias(int64 data, char **param);
void G_DoOptimizeActionTree(void);

alist_t    *CurrentActions[MAX_CURRENTACTIONS];

//these must be in the order key, joy, mouse, mouse2, other
#define KEY_ACTIONPOS       0
#define MOUSE_ACTIONPOS     NUMKEYS
#define MOUSE2_ACTIONPOS    (MOUSE_ACTIONPOS+MOUSE_BUTTONS)
#define NUM_ACTIONS         (MOUSE2_ACTIONPOS+MOUSE_BUTTONS)

alist_t    *AllActions[NUM_ACTIONS];

alist_t    **KeyActions;
alist_t    **MouseActions;
alist_t    **Mouse2Actions;

static int  MouseButtons = 0;

static dboolean OptimizeTree = false;
dboolean        ButtonAction = false;

static CMD(Alias);
static CMD(Unbind);
static CMD(UnbindAll);

//
// G_InitActions
//

void G_InitActions(void) {
    dmemset(AllActions, 0, NUM_ACTIONS);
    KeyActions = AllActions + KEY_ACTIONPOS;
    MouseActions = AllActions + MOUSE_ACTIONPOS;
    Mouse2Actions = AllActions + MOUSE2_ACTIONPOS;

    dmemset(CurrentActions, 0, MAX_CURRENTACTIONS*sizeof(alist_t *));

    G_AddCommand("alias", CMD_Alias, 0);
    G_AddCommand("unbind", CMD_Unbind, 0);
    G_AddCommand("unbindall", CMD_UnbindAll, 0);
}

//
// FindAction
//

static action_t *FindAction(char *name) {
    action_t    *tree;
    int         cmp;

    if(!name) {
        return NULL;
    }

    tree = Actions;
    while(tree) {
        cmp = dstrcmp(name, tree->name);
        if(cmp == 0) {
            break;
        }

        if(cmp > 0) {
            tree = tree->children[1];
        }
        else {
            tree = tree->children[0];
        }
    }

    return tree;
}

//
// SkipWhitespace
//

char *SkipWhitespace(char *p) {
    while(*p && isspace(*p)) {
        p++;
    }
    return p;
}

//
// FindWhitespace
//

char *FindWhitespace(char *p) {
    while(*p && !isspace(*p)) {
        p++;
    }

    return p;
}

//
// DuplicateActionList
//

alist_t *DuplicateActionList(alist_t *al) {
    alist_t *p;

    p = al;
    while(p) {
        p->refcount++;
        p = p->next;
    }

    return al;
}

//
// AddActions
//

void AddActions(alist_t *actions) {
    int slot;

    if(!actions) {
        return;
    }

    for(slot = 0; slot < MAX_CURRENTACTIONS; slot++) {
        if(!CurrentActions[slot]) {
            CurrentActions[slot] = DuplicateActionList(actions);
            break;
        }
    }
    if(slot == MAX_CURRENTACTIONS) {
        CON_Warnf("command overflow\n");
    }
}

//
// DerefSingleAction
//

void DerefSingleAction(alist_t *al) {
    al->refcount--;
    if(al->refcount <= 0) {
        if(al->buff) {
            if(al->next) {
                al->next->buff = al->buff;
            }
            else {
                Z_Free(al->buff);
            }
        }

        Z_Free(al);
    }
}

//
// DoRunActions
// Executes either a command or cvar
// Matches user input to see if its valid
//

alist_t *DoRunActions(alist_t *al, dboolean free) {
    alist_t     *next = NULL;
    action_t    *action;

    while(al) {
        next = al->next;
        if(dstrcmp(al->cmd, "wait") == 0) {
            break;
        }

        action = FindAction(al->cmd);
        if(action) {
            action->proc(action->data, al->param);
        }
        else if (auto cvar = Cvar::find(al->cmd)) {
            // FIXME: Netgame cvar setting
#if 0
            if(netgame) {
                if(cvar->nonclient) {
                    // I'll just have to assume for now that
                    // player# 0 is the server..
                    if(consoleplayer != 0) {
                        CON_Warnf("Cannot change cvar that's locked by server\n");
                        goto next;
                    }
                }
            }
#endif

            if(!al->param[0]) {
                auto str = fmt::format("{}: {} ({})", cvar->name(), cvar->string(), cvar->default_string());
                CON_AddLine(str.c_str(), str.size());
            }
            else {
                cvar->set_string(al->param[0]);
                // FIXME: Update cvar in network game
#if 0
                if(netgame) {
                    if(playeringame[0] && consoleplayer == 0) {
                        NET_SV_UpdateCvars(cvar);
                    }
                }
#endif
            }
        }
        else {
            CON_Warnf("Unknown command \"%s\"\n", al->cmd);
        }

        if(free) {
            DerefSingleAction(al);
        }

        al = next;
    }

    if(al) { // reached wait command
        if(free) {
            DerefSingleAction(al);
        }

        if(next != NULL) {
            return next;
        }
    }

    return al;
}

//
// TryActions
//

void TryActions(alist_t *al, dboolean up) {
    if(!al) {
        return;
    }

    if(up) {
        action_t    *action;
        char        buff[256];

        if(al->next || (al->cmd[0] != '+')) {
            return;
        }

        dstrcpy(buff, al->cmd);
        buff[0] = '-';
        action = FindAction(buff);
        if(action) {
            action->proc(action->data, al->param);
        }

        return;
    }

    AddActions(DoRunActions(al, false));
}

//
// DerefActionList
//

void DerefActionList(alist_t *al) {
    alist_t *next;

    while(al) {
        next = al->next;
        al->refcount--;
        if(al->refcount <= 0) {
            if(al->buff) {
                Z_Free(al->buff);
            }

            Z_Free(al);
        }

        al = next;
    }
}

//
// G_ActionTicker
//

void G_ActionTicker(void) {
    int slot;

    for(slot = 0; slot < MAX_CURRENTACTIONS; slot++) {
        if(CurrentActions[slot]) {
            CurrentActions[slot] = DoRunActions(CurrentActions[slot], true);
        }
    }

    if(OptimizeTree) {
        G_DoOptimizeActionTree();

        // 20120310 villsa - do we really need to optimize this every tick?
        // set back to false when we're done
        OptimizeTree = false;
    }
}

//
// ProcessButtonActions
//

static void ProcessButtonActions(alist_t **actions, int b, int ob) {
    int mask;

    ButtonAction = true;
    ob ^= b;
    for(mask = 1; mask; mask <<= 1, actions++) {
        if(ob & mask) {
            TryActions(*actions, (b & mask) == 0);
        }
    }

    ButtonAction = false;
}

//
// G_ActionResponder
//

dboolean G_ActionResponder(event_t *ev) {
    switch(ev->type) {
    case ev_keyup:
    case ev_keydown:
        if((ev->data1 < 0) || (ev->data1 >= NUMKEYS)) {
            break;
        }

        TryActions(KeyActions[ev->data1], ev->type == ev_keyup);
        break;

    // villsa 12/10/2013: properly handle mouse button actions and
    // handle mouse movement in its own event state
    case ev_mousedown:
    case ev_mouseup:
        ProcessButtonActions(MouseActions, ev->data1, MouseButtons);
        MouseButtons = ev->data1;

        // MP2E 12/10/2013: ev_mousedown and mouseup need G_DoCmdMouseMove
        G_DoCmdMouseMove(ev->data2, ev->data3);
        break;

    case ev_mouse:
        G_DoCmdMouseMove(ev->data2, ev->data3);
        break;

#if 0
    case ev_gamepad:
        G_DoCmdGamepadMove(ev);
        break;
#endif
    }

    return false;
}

//
// NextToken
//

char* NextToken(char *s, dboolean *pquoted) { //null terminates current token
    char *p;

    p = s;
    if(*pquoted) {
        while(*p && (*p != '"')) {
            p++;
        }

        if(*p == '"') {
            *(p++) = 0;
        }
    }
    while(*p && (*p != ';') && !isspace(*p)) {
        p++;
    }

    if(isspace(*p)) {
        *(p++) = 0;
        p = SkipWhitespace(p);
    }
    if(*p == '"') {
        *pquoted = true;
        p++;
    }
    else {
        *pquoted = false;
    }

    return p;
}

//
// ParseActions
//

alist_t* ParseActions(char *actions) {
    char        *p;
    int         param;
    alist_t     *al;
    alist_t     *alist;
    dboolean    quoted;

    if(!actions) {
        return NULL;
    }

    p = SkipWhitespace(actions);

    if(!*p) {
        return NULL;
    }

    alist = (alist_t *)Z_Malloc(sizeof(alist_t), PU_STATIC, NULL);
    al = alist;
    al->buff = strdup(p);
    p = al->buff;
    quoted = false;

    while(true) {
        al->cmd = p;
        al->refcount = 1;
        param = 0;
        p = NextToken(p, &quoted);

        while(*p && (*p != ';')) {
            if(param < MAX_ACTIONPARAM) {
                al->param[param++] = p;
            }

            p = NextToken(p, &quoted);
        }
        while(param <= MAX_ACTIONPARAM) {
            al->param[param++]=NULL;
        }

        while(*p == ';') {
            *(p++) = 0;
            p = SkipWhitespace(p);
        }

        dstrlwr(al->cmd);

        if(!*p) {
            al->next = NULL;
            return alist;
        }
        al->next = (alist_t *)Z_Malloc(sizeof(alist_t), PU_STATIC, NULL);
        al = al->next;
        al->buff = NULL;
    }
}

//
// G_ExecuteCommand
//

void G_ExecuteCommand(char *action) {
    alist_t *al;

    al = ParseActions(action);
    al = DoRunActions(al, true);
    if(al) {
        AddActions(al);
        DerefActionList(al);
    }
}

//
// FindActionControler
//

alist_t **FindActionControler(char *name, alist_t **actions, int numbuttons) {
    int i;

    if((name[0] >= '1') && (name[0] <= '9')) {
        i = datoi(name)-1;
        if(i > numbuttons) {
            return NULL;
        }
    }
    else {
        return NULL;
    }

    return(&actions[i]);
}

//
// G_FindKeyByName
//

alist_t **G_FindKeyByName(char *key) {
    int     i;
    char    buff[MAX_KEY_NAME_LENGTH];

    if(dstrncmp(key, "mouse", 5) == 0) {
        //gets confused if have >20 mouse buttons:)
        if((key[5] == '2') && key[6]) {
            return(FindActionControler(&key[6], Mouse2Actions, MOUSE_BUTTONS));
        }
        return(FindActionControler(&key[5], MouseActions, MOUSE_BUTTONS));
    }

    for(i = 0; i < NUMKEYS; i++) {
        M_GetKeyName(buff, i);
        if(dstricmp(key, buff) == 0) {
            return(&KeyActions[i]);
        }
    }

    return NULL;
}

//
// G_BindAction
//

void G_BindAction(alist_t **plist, char *action) {
    alist_t    *al;

    al = ParseActions(action);

    if(plist) {
        if(*plist) {
            DerefActionList(*plist);
        }
        *plist = al;
    }
    else {
        CON_Warnf("Unknown Key\n");
        DerefActionList(al);
    }
}

//
// G_BindActionByEvent
//

static int GetBitNum(int bits) {
    int mask;
    int i;

    for(mask = 1, i = 0; mask; mask <<= 1, i++) {
        if(mask & bits) {
            return i;
        }
    }
    return -1;
}

dboolean G_BindActionByEvent(event_t *ev, char *action) {
    int     button;
    alist_t **plist;

    plist = NULL;
    switch(ev->type) {
    case ev_keydown:
        //
        // HACK: ignore capslock
        //
        if(ev->data1 == KEY_CAPS) {
            return false;
        }

        plist = &KeyActions[ev->data1];
        break;

    case ev_mouse:
    case ev_mousedown:
        button = GetBitNum(ev->data1);
        if((button >= 0) && (button < MOUSE_BUTTONS)) {
            plist = &MouseActions[button];
        }
        break;

    case ev_keyup:
    case ev_mouseup:
    case ev_gamepad:
        break;
    }
    if(plist) {
        G_BindAction(plist, action);
        return true;
    }
    return false;
}

//
// G_BindActionByName
//

void G_BindActionByName(char *key, char *action) {
    G_BindAction(G_FindKeyByName(key), action);
}

//
// G_OutputBindings
//

static void OutputActions(FILE * fh, alist_t *al, char *name) {
    int i;

    fprintf(fh, "bind %s \"", name);
    while(al) {
        fprintf(fh, "%s", al->cmd);
        for(i=0; al->param[i]; i++) {
            fprintf(fh, " %s", al->param[i]);
        }
        al = al->next;
        if(al) {
            fprintf(fh, " ; ");
        }
    }
    fprintf(fh, "\"\n");
}

void G_OutputBindings(FILE *fh) {
    int         i;
    alist_t     *al;
    char        name[MAX_KEY_NAME_LENGTH];

    for(i = 0; i < NUMKEYS; i++) {
        al = KeyActions[i];
        if(!al) {
            continue;
        }

        M_GetKeyName(name, i);
        OutputActions(fh, al, name);
    }

    dstrcpy(name, "mouse");

    for(i = 0; i < MOUSE_BUTTONS; i++) {
        al = MouseActions[i];
        if(al) {
            name[5] = i+'1';
            name[6] = 0;
            OutputActions(fh, al, name);
        }
    }
    for(i = 0; i< MOUSE_BUTTONS; i++) {
        al = Mouse2Actions[i];
        if(al) {
            name[5] = '2';
            name[6] = i+'1';
            name[7] = 0;
            OutputActions(fh, al, name);
        }
    }

    // cvars
    Cvar::write_config(fh);
}

//
// G_PrintActions
//

void G_PrintActions(alist_t *al) {
    int i;

    while(al) {
        I_Printf("%s", al->cmd);
        for(i = 0; al->param[i]; i++) {
            I_Printf(" %s", al->param[i]);
        }
        al = al->next;
        if(al) {
            I_Printf(" ; ");
        }
    }
}

//
// G_ShowBinding
//

void G_ShowBinding(char *key) {
    alist_t **alist;

    alist = G_FindKeyByName(key);
    if(!alist) {
        CON_Warnf("Unknown key:%s\n", key);
        return;
    }
    if(!*alist) {
        CON_Warnf("%s is not bound\n", key);
        return;
    }

    I_Printf("%s = ", key);
    G_PrintActions(*alist);
    I_Printf("\n");
}

static int      NumActions;
static action_t **ActionBuffer;

int CountActions(action_t *action) {
    int num;

    if(!action) {
        return 0;
    }

    num = 1;
    if(action->children[0]) {
        num += CountActions(action->children[0]);
    }
    if(action->children[1]) {
        num += CountActions(action->children[1]);
    }

    return num;
}

void DumpActions(action_t *action) {
    if(!action) {
        return;
    }

    if(action->children[0]) {
        DumpActions(action->children[0]);
    }

    ActionBuffer[NumActions++] = action;

    if(action->children[1]) {
        DumpActions(action->children[1]);
    }
}

action_t *RebuildActions(int left, int right) {
    int         mid;
    action_t    *action;

    mid = (left + right) / 2;
    action = ActionBuffer[mid];

    if(left<mid) {
        action->children[0] = RebuildActions(left, mid-1);
        action->children[0]->parent = action;
    }
    else {
        action->children[0] = NULL;
    }

    if(mid<right) {
        action->children[1] = RebuildActions(mid+1, right);
        action->children[1]->parent = action;
    }
    else {
        action->children[1] = NULL;
    }
    return action;
}

//
// G_OptimizeActionTree
//

void G_OptimizeActionTree(void) {
    OptimizeTree = true;
}

//
// G_DoOptimizeActionTree
// dumps into array, then rebuilds tree to minimize tree depth
//

void G_DoOptimizeActionTree(void) {
    int count;

    // lots of nice recursive procedures :)

    count = CountActions(Actions);
    if(count == 0) {
        return;
    }

    ActionBuffer = (action_t **)Z_Malloc(count*sizeof(action_t *), PU_STATIC, NULL);
    NumActions = 0;
    DumpActions(Actions);

    Actions = RebuildActions(0, count-1);
    Actions->parent = NULL;
    Z_Free(ActionBuffer);
}

//
// G_FreeAction
// does not free children, or remove from tree
//

void G_FreeAction(action_t *action) {
    if(!action) {
        return;
    }

    if(action->name) {
        Z_Free(action->name);
    }

    if(action->proc == G_RunAlias) {
        DerefActionList((alist_t *)action->data);
    }

    Z_Free(action);
}

//does not alter with->children
static void ReplaceActionWith(action_t *action, action_t *with) {
    if(action == with) {
        return;
    }

    if(!action) {
        I_Error("Replaced NULL action");
    }

    if(action->parent) {
        if(action->parent->children[0] == action) {
            action->parent->children[0] = with;
        }
        else {
            action->parent->children[1] = with;
        }
    }
    else {
        Actions = with;
    }

    if(with) {
        with->parent = action->parent;
    }
}

static void AddAction(action_t *action) {
    action_t    *tree;
    int         cmp;
    int         child;

    action->parent = NULL;
    action->children[0] = action->children[1] = NULL;

    if(Actions) {
        tree = Actions;
        while(tree) {
            cmp = dstrcmp(action->name, tree->name);
            if(cmp == 0) {
                ReplaceActionWith(tree, action);

                for(child = 0; child < 2; child++) {
                    action->children[child] = tree->children[child];
                    if(tree->children[child]) {
                        tree->children[child]->parent = action;
                    }
                    tree->children[child] = NULL;
                }

                G_FreeAction(tree);
                tree = NULL;
            }
            else {
                if(cmp>0) {
                    child = 1;
                }
                else {
                    child = 0;
                }
                if(tree->children[child]) {
                    tree = tree->children[child];
                }
                else {
                    action->parent = tree;
                    tree->children[child] = action;
                    tree = NULL;
                    G_OptimizeActionTree();
                }
            }
        }
    }
    else {
        Actions = action;
    }
}

//
// G_AddCommand
// Adds a new action to the list
//

void G_AddCommand(const char *name, actionproc_t proc, int64 data) {
    action_t *action;

    action = (action_t *)Z_Malloc(sizeof(action_t), PU_STATIC, NULL);
    action->name = strdup(name);
    dstrlwr(action->name);
    action->proc = proc;
    action->data = data;
    AddAction(action);
}

//
// G_RunAlias
//

void G_RunAlias(int64 data, char **param) {
    AddActions(DoRunActions((alist_t *)data, false));
}

//
// G_ShowAliases
//

void G_ShowAliases(action_t *action) {
    if(!action) {
        return;
    }

    if(action->children[0]) {
        G_ShowAliases(action->children[0]);
    }

    if((action->proc == G_RunAlias) && action->data) {
        I_Printf(" %s = ", action->name);
        G_PrintActions((alist_t *)action->data);
        I_Printf("\n");
    }

    if(action->children[1]) {
        G_ShowAliases(action->children[1]);
    }
}

//
// G_UnregisterAction
//

void G_UnregisterAction(char *name) {
    action_t    *action;
    action_t    *tree;
    char        buff[256];

    dstrcpy(buff, name);
    dstrlwr(buff);

    action = FindAction(buff);

    if(!action) {
        return;
    }

    if(!action->children[0]) {
        ReplaceActionWith(action, action->children[1]);
    }
    else if(!action->children[1]) {
        ReplaceActionWith(action, action->children[0]);
    }
    else {
        tree = action->children[1];
        while(tree->children[0]) {
            tree = tree->children[0];
        }

        tree->children[0] = action->children[0];

        action->children[0]->parent = tree;
        G_OptimizeActionTree();
    }
    G_FreeAction(action);
}

//
// CMD_Alias
//

static CMD(Alias) {
    alist_t *al;

    if(!param[0]) {
        I_Printf("Current Aliases:\n");
        G_ShowAliases(Actions);
        return;
    }
    al = ParseActions(param[1]);
    if(!al) {
        G_UnregisterAction(param[0]);
    }
    else {
        G_AddCommand(param[0], G_RunAlias, (int64)al);
    }
}

//
// G_ListCommands
//

static int ListCommandRecurse(action_t *action) {
    int count;

    if(!action) {
        return 0;
    }

    count = 1;
    if(action->children[0]) {
        count += ListCommandRecurse(action->children[0]);
    }

    if(action->name[0] == '-') {
        count--;
    }
    else {
        CON_Printf(AQUA, " %s\n", action->name);
    }

    if(action->children[1]) {
        count += ListCommandRecurse(action->children[1]);
    }

    return(count);
}

//
// G_ListCommands
//

int G_ListCommands(void) {
    return(ListCommandRecurse(Actions));
}

//
// Unbind
//

static void Unbind(char *action) {
    alist_t **alist;

    alist = G_FindKeyByName(action);
    if(!alist) {
        I_Printf("Unknown Key:%s\n", action);
        return;
    }
    if(*alist) {
        DerefActionList(*alist);
        *alist=NULL;
    }
}

//
// CMD_Unbind
//

static CMD(Unbind) {
    if(!param[0]) {
        I_Printf(" unbind <key>\n");
        return;
    }

    Unbind(param[0]);
}

//
// UnbindActions
//

static void UnbindActions(alist_t **alist, int num) {
    int i;

    for(i = 0; i < num; i++) {
        if(!alist[i]) {
            continue;
        }

        DerefActionList(*alist);
        return;
    }
}


//
// CMD_UnbindAll
//

static CMD(UnbindAll) {
    UnbindActions(AllActions, NUM_ACTIONS);
}

//
// IsSameAction
//

static dboolean IsSameAction(const char *cmd, alist_t *al) {
    if(!al) {
        return false;
    }

    if(!dstrcmp(al->cmd, "weapon")) {
        {
            char buff[256];
            sprintf(buff, "%s %s", al->cmd, al->param[0]);
            return strcmp(cmd, buff) == 0;
        }
    }
    else {
        if(dstricmp(cmd, al->cmd)!=0) {
            return false;
        }
    }
    return true;
}

//
// G_GetActionName
//

void G_GetActionName(char *buff, int n) {
    *buff = 0;
    if(n >= NUM_ACTIONS) {
        return;
    }

    if(n >= MOUSE2_ACTIONPOS) {
        sprintf(buff, "mouse2%d", n-MOUSE2_ACTIONPOS);
        return;
    }
    if(n >= MOUSE_ACTIONPOS) {
        sprintf(buff, "mouse2%d", n-MOUSE_ACTIONPOS);
        return;
    }
    if(n >= KEY_ACTIONPOS) {
        M_GetKeyName(buff, n-KEY_ACTIONPOS);
        return;
    }
}

//
// G_GetActionBindings
//

void G_GetActionBindings(char *buff, const char *action) {
    int     i;
    char    *p;

    p = buff;
    *p = 0;
    for(i = 0; i < NUMKEYS; i++) {
        if(IsSameAction(action, KeyActions[i])) {
            if(p != buff) {
                *(p++) = ',';
            }

            M_GetKeyName(p, i);

            p += dstrlen(p);

            if(p - buff >= MAX_MENUACTION_LENGTH) {
                return;
            }
        }
    }
    for(i = 0; i < MOUSE_BUTTONS; i++) {
        if(IsSameAction(action, MouseActions[i])) {
            if(p != buff) {
                *(p++) = ',';
            }

            if(i < MOUSE_BUTTONS-2) {
                dstrcpy(p, "mouse?");
                p[5] = i + '1';
                p += 6;
            }

            if(p - buff >= MAX_MENUACTION_LENGTH) {
                return;
            }
        }
        if(IsSameAction(action, Mouse2Actions[i])) {
            if(p != buff) {
                *(p++) = ',';
            }
            dstrcpy(p, "mouse2?");
            p[6] = i + '1';
            p += 7;
            if(p - buff >= MAX_MENUACTION_LENGTH) {
                return;
            }
        }
    }
}

//
// G_UnbindAction
//

void G_UnbindAction(const char *action) {
    int i;

    for(i = 0; i < NUMKEYS; i++) {
        if(IsSameAction(action, KeyActions[i])) {
            char p[16];

            M_GetKeyName(p, i);
            Unbind(p);
            return;
        }
    }
    for(i = 0; i < MOUSE_BUTTONS; i++) {
        if(IsSameAction(action, MouseActions[i])) {
            char p[16];

            if(i < MOUSE_BUTTONS-2) {
                dstrcpy(p, "mouse?");
                p[5] = i + '1';
            }

            Unbind(p);
            return;
        }
        if(IsSameAction(action, Mouse2Actions[i])) {
            char p[16];

            dstrcpy(p, "mouse2?");
            p[6] = i + '1';

            Unbind(p);
            return;
        }
    }
}

