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
// DESCRIPTION: Main console functions
//
//-----------------------------------------------------------------------------

#include "doomstat.h"
#include "con_console.h"
#include "z_zone.h"
#include "st_stuff.h"
#include "g_actions.h"
#include "m_shift.h"
#include "gl_draw.h"
#include "r_main.h"
#include "i_system.h"
#include "gl_texture.h"

#define CONSOLE_PROMPTCHAR      '>'
#define MAX_CONSOLE_LINES       256//must be power of 2
#define CONSOLETEXT_MASK        (MAX_CONSOLE_LINES-1)
#define CMD_HISTORY_SIZE        64
#define CONSOLE_Y               160

typedef struct {
    int    len;
    dword  color;
    char   line[1];
} conline_t;

enum {
    CST_UP,
    CST_RAISE,
    CST_LOWER,
    CST_DOWN
};

#define     CON_BUFFERSIZE  100

static conline_t    **console_buffer;
static int          console_head;
static int          console_lineoffset;
static int          console_minline;
static dboolean     console_enabled = false;
static int          console_pos = 0;//bottom of console, in pixels
static char         console_linebuffer[CON_BUFFERSIZE];
static int          console_linelength;
static int          console_state = CST_UP;
static int          console_prevcmds[CMD_HISTORY_SIZE];
static int          console_cmdhead;
static int          console_nextcmd;

char        console_inputbuffer[MAX_CONSOLE_INPUT_LEN];
int         console_inputlength;
int     console_autocomplete = 0;
bool    console_initialized = false;
std::vector<Property*> console_autocomplete_list;

//
// CON_AutoComplete
//
static void CON_AutoComplete()
{
    if (console_inputlength <= 1)
        return;

    if (console_autocomplete == 0) {
        console_autocomplete_list = Property::partial(&console_inputbuffer[1]);
    }

    if (console_autocomplete_list.empty())
        return;

    auto index = (console_autocomplete++) % console_autocomplete_list.size();
    auto entry = console_autocomplete_list[index]->name();
    strcpy(console_inputbuffer + 1, entry.data());
    console_inputlength = entry.length() + 1;
}
//
// CON_Init
//

void CON_Init(void) {
    int i;

    console_buffer = (conline_t **)Z_Malloc(sizeof(conline_t *) * MAX_CONSOLE_LINES, PU_STATIC, NULL);
    console_head = 0;
    console_minline = 0;
    console_lineoffset = 0;

    for(i = 0; i < MAX_CONSOLE_LINES; i++) {
        console_buffer[i] = NULL;
    }

    for(i = 0; i < MAX_CONSOLE_INPUT_LEN; i++) {
        console_inputbuffer[i] = 0;
    }

    console_linelength = 0;
    console_inputlength = 1;
    console_inputbuffer[0] = CONSOLE_PROMPTCHAR;

    for(i = 0; i < CMD_HISTORY_SIZE; i++) {
        console_prevcmds[i] = -1;
    }

    console_cmdhead = 0;
    console_nextcmd = 0;

#ifdef USESYSCONSOLE
    {
        extern HWND hwndBuffer;
        char *buff;
        int i = SendMessage(hwndBuffer, WM_GETTEXTLENGTH, 0, 0);

        buff = Z_Alloca(i);

        SendMessage(hwndBuffer, WM_GETTEXT, i, (LPARAM)buff);
        CON_AddText(buff);
    }
#endif

    console_initialized = true;
}

//
// CON_AddLine
//

void CON_AddLine(const char *line, int len) {
    conline_t   *cline;
    int         i;
    dboolean    recursed = false;

    if(!console_linebuffer) {
        //not initialised yet
        return;
    }

    if(recursed) {
        //later call to Z_Malloc can fail and call I_Error/I_Printf...
        return;
    }

    recursed = true;

    if(!line) {
        return;
    }

    if(len == -1) {
        len = dstrlen(line);
    }

    cline = (conline_t *)Z_Malloc(sizeof(conline_t)+len, PU_STATIC, NULL);
    cline->len = len;

    if(len) {
        dmemcpy(cline->line, line, len);
    }

    cline->line[len] = 0;
    console_head = (console_lineoffset + CONSOLETEXT_MASK) & CONSOLETEXT_MASK;
    console_minline = console_head;
    console_lineoffset = console_head;

    console_buffer[console_head] = cline;
    console_buffer[console_head]->color = WHITE;

    i = (console_head + CONSOLETEXT_MASK) & CONSOLETEXT_MASK;
    if(console_buffer[i]) {
        Z_Free(console_buffer[i]);
        console_buffer[i] = NULL;
    }

    recursed = false;
}

//
// CON_AddText
//

void CON_AddText(const char *text) {
    const char *src;
    char    c;

    if(!console_linebuffer) {
        return;
    }

    src = text;
    c = *(src++);
    while(c) {
#ifdef USESYSCONSOLE
        if(*src == '\r' && *(src + 1) == '\n') {
            *src = 0x20;
            *(src + 1) = '\n';
        }
        else if(*src == '\r') {
            *src = '\n';
        }
#endif

        if((c == '\n') || (console_linelength >= CON_BUFFERSIZE)) {
            CON_AddLine(console_linebuffer, console_linelength);
            console_linelength = 0;
        }
        if(c != '\n') {
            console_linebuffer[console_linelength++] = c;
        }

        c = *(src++);
    }
}

//
// CON_Printf
//

void CON_Printf(rcolor clr, const char *s, ...) {
    static char msg[MAX_MESSAGE_SIZE];
    va_list    va;

    va_start(va, s);
    vsprintf(msg, s, va);
    va_end(va);

    I_Printf("%s", msg);
    console_buffer[console_head]->color = clr;
}

//
// CON_Warnf
//

void CON_Warnf(const char *s, ...) {
    static char msg[MAX_MESSAGE_SIZE];
    va_list    va;

    va_start(va, s);
    vsprintf(msg, s, va);
    va_end(va);

    CON_Printf(YELLOW, "WARNING: ");
    CON_Printf(YELLOW, "%s", msg);
}

//
// CON_DPrintf
//

void CON_DPrintf(const char *s, ...) {
    if(devparm) {
        static char msg[MAX_MESSAGE_SIZE];
        va_list    va;

        va_start(va, s);
        vsprintf(msg, s, va);
        va_end(va);

        CON_Printf(RED, "%s", msg);
    }
}

//
// CON_ParseKey
//

static dboolean shiftdown = false;

void CON_ParseKey(char c) {
    if(c < ' ') {
        return;
    }

    if(c == KEY_BACKSPACE) {
        if(console_inputlength > 1) {
            console_inputbuffer[--console_inputlength] = 0;
        }

        return;
    }

    if(shiftdown) {
        c = shiftxform[c];
    }

    if(console_inputlength >= MAX_CONSOLE_INPUT_LEN - 2) {
        console_inputlength = MAX_CONSOLE_INPUT_LEN - 2;
    }

    console_inputbuffer[console_inputlength++] = c;
}

//
// CON_Ticker
//

static dboolean keyheld = false;
static dboolean lastevent = 0;
static int lastkey = 0;
static int ticpressed = 0;

void CON_Ticker(void) {
    if(!console_enabled) {
        return;
    }

    if(keyheld && ((gametic - ticpressed) >= 15)) {
        CON_ParseKey(lastkey);
    }
}

//
// CON_Responder
//

void G_ClearInput(void);

dboolean CON_Responder(event_t* ev) {
    int c;
    dboolean clearheld = true;

    if((ev->type != ev_keyup) && (ev->type != ev_keydown)) {
        return false;
    }

    c = ev->data1;
    lastkey = c;
    lastevent = ev->type;

    if(ev->type == ev_keydown && !keyheld) {
        keyheld = true;
        ticpressed = gametic;
    }
    else {
        keyheld = false;
        ticpressed = 0;
    }

    if(c == KEY_SHIFT) {
        if(ev->type == ev_keydown) {
            shiftdown = true;
        }
        else if(ev->type == ev_keyup) {
            shiftdown = false;
        }
    }

    switch(console_state) {
    case CST_DOWN:
    case CST_LOWER:
        if(ev->type == ev_keydown) {
            switch(c) {
            case '`':
            case '^':
                console_state = CST_UP;
                console_enabled = false;
                break;

            case KEY_ESCAPE:
                console_inputlength = 1;
                console_autocomplete = 0;
                break;

            case KEY_TAB:
                CON_AutoComplete();
                break;

            case KEY_ENTER:
                if(console_inputlength <= 1) {
                    break;
                }

                console_inputbuffer[console_inputlength]=0;
                CON_AddLine(console_inputbuffer, console_inputlength);

                console_prevcmds[console_cmdhead] = console_head;
                console_cmdhead++;
                console_nextcmd = console_cmdhead;

                if(console_cmdhead >= CMD_HISTORY_SIZE) {
                    console_cmdhead = 0;
                }

                console_prevcmds[console_cmdhead] = -1;
                G_ExecuteCommand(&console_inputbuffer[1]);
                console_inputlength = 1;
                console_autocomplete = 0;
                CONCLEARINPUT();
                break;

            case KEY_UPARROW:
                c = console_nextcmd - 1;
                if(c < 0) {
                    c = CMD_HISTORY_SIZE - 1;
                }

                if(console_prevcmds[c] == -1) {
                    break;
                }

                console_nextcmd = c;
                c = console_prevcmds[console_nextcmd];
                if(console_buffer[c]) {
                    console_inputlength = console_buffer[c]->len;
                    dmemcpy(console_inputbuffer, console_buffer[console_prevcmds[console_nextcmd]]->line, console_inputlength);
                }
                console_autocomplete = 0;
                break;

            case KEY_DOWNARROW:
                if(console_prevcmds[console_nextcmd] == -1) {
                    break;
                }

                c = console_nextcmd + 1;
                if(c >= CMD_HISTORY_SIZE) {
                    c = 0;
                }

                if(console_prevcmds[c] == -1) {
                    break;
                }

                console_nextcmd = c;
                console_inputlength = console_buffer[console_prevcmds[console_nextcmd]]->len;
                dmemcpy(console_inputbuffer, console_buffer[console_prevcmds[console_nextcmd]]->line, console_inputlength);
                console_autocomplete = 0;
                break;

            case KEY_MWHEELUP:
            case KEY_PAGEUP:
                if(console_head < MAX_CONSOLE_LINES) {
                    console_head++;
                }
                break;

            case KEY_MWHEELDOWN:
            case KEY_PAGEDOWN:
                if(console_head > console_minline) {
                    console_head--;
                }
                break;

            default:
                if(c == KEY_SHIFT || c == KEY_ALT || c == KEY_CTRL) {
                    break;
                }

                clearheld = false;
                CON_ParseKey(c);
                console_autocomplete = 0;
                break;
            }

            if(clearheld) {
                keyheld = false;
                ticpressed = 0;
            }
        }
        return true;

    case CST_UP:
    case CST_RAISE:
        if(c == '`' || c == '^') {
            if(ev->type == ev_keydown) {
                console_state = CST_DOWN;
                console_enabled = true;
                G_ClearInput();
            }
            return false;
        }
        break;
    }

    return false;
}

//
// CON_Draw
//

#define CONFONT_SCALE   ((float)(video_width * SCREENHEIGHT) / (float)(video_width * video_height))
#define CONFONT_YPAD    (16 * CONFONT_SCALE)

void CON_Draw(void) {
    int     line;
    float   y = 0;
    float   x = 0;
    float   inputlen;

    if(!console_linebuffer) {
        return;
    }

    if(!console_enabled) {
        return;
    }

    GL_SetOrtho(1);
    GL_SetState(GLSTATE_BLEND, 1);

    dglDisable(GL_TEXTURE_2D);
    dglColor4ub(0, 0, 0, 128);
    dglRectf(SCREENWIDTH, CONSOLE_Y + CONFONT_YPAD, 0, 0);

    GL_SetState(GLSTATE_BLEND, 0);

    dglColor4f(0, 1, 0, 1);
    dglBegin(GL_LINES);
    dglVertex2f(0, CONSOLE_Y - 1);
    dglVertex2f(SCREENWIDTH, CONSOLE_Y - 1);
    dglVertex2f(0, CONSOLE_Y + CONFONT_YPAD);
    dglVertex2f(SCREENWIDTH, CONSOLE_Y + CONFONT_YPAD);
    dglEnd();
    dglEnable(GL_TEXTURE_2D);

    line = console_head;

    y = CONSOLE_Y - 2;

    if(line < MAX_CONSOLE_LINES) {
        while(console_buffer[line] && y > 0) {
            Draw_ConsoleText(0, y, console_buffer[line]->color, CONFONT_SCALE, "%s", console_buffer[line]->line);
            line = (line + 1) & CONSOLETEXT_MASK;
            y -= CONFONT_YPAD;
        }
    }

    y = CONSOLE_Y + CONFONT_YPAD;

    inputlen = Draw_ConsoleText(x, y, WHITE, CONFONT_SCALE, "%s", console_inputbuffer);
    Draw_ConsoleText(x + inputlen, y, WHITE, CONFONT_SCALE, "_");
}
