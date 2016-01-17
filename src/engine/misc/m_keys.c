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
// DESCRIPTION: Key binding strings
//
//-----------------------------------------------------------------------------

#include "m_keys.h"
#include "doomdef.h"
#include "i_xinput.h"

typedef struct {
    int        code;
    char    *name;
} keyinfo_t;

static keyinfo_t    Keys[]= {
    {KEY_RIGHTARROW,        "Right"},
    {KEY_LEFTARROW,         "Left"},
    {KEY_UPARROW,           "Up"},
    {KEY_DOWNARROW,         "Down"},
    {KEY_ESCAPE,            "Escape"},
    {KEY_ENTER,             "Enter"},
    {KEY_TAB,               "Tab"},
    {KEY_BACKSPACE,         "Backsp"},
    {KEY_PAUSE,             "Pause"},
    {KEY_SHIFT,             "Shift"},
    {KEY_ALT,               "Alt"},
    {KEY_CTRL,              "Ctrl"},
    {KEY_EQUALS,            "+"},
    {KEY_MINUS,             "-"},
    {KEY_ENTER,             "Enter"},
    {KEY_CAPS,              "Caps"},
    {KEY_INSERT,            "Ins"},
    {KEY_DEL,               "Del"},
    {KEY_HOME,              "Home"},
    {KEY_END,               "End"},
    {KEY_PAGEUP,            "PgUp"},
    {KEY_PAGEDOWN,          "PgDn"},
    {';',                   ";"},
    {'\'',                  "'"},
    {'#',                   "#"},
    {'\\',                  "\\"},
    {',',                   ","},
    {'.',                   "."},
    {'/',                   "/"},
    {'[',                   "["},
    {']',                   "]"},
    {'*',                   "*"},
    {' ',                   "Space"},
    {KEY_F1,                "F1"},
    {KEY_F2,                "F2"},
    {KEY_F3,                "F3"},
    {KEY_F4,                "F4"},
    {KEY_F5,                "F5"},
    {KEY_F6,                "F6"},
    {KEY_F7,                "F7"},
    {KEY_F8,                "F8"},
    {KEY_F9,                "F9"},
    {KEY_F10,               "F10"},
    {KEY_F11,               "F11"},
    {KEY_F12,               "F12"},
    {KEY_KEYPADENTER,       "KeyPadEnter"},
    {KEY_KEYPADMULTIPLY,    "KeyPad*"},
    {KEY_KEYPADPLUS,        "KeyPad+"},
    {KEY_NUMLOCK,           "NumLock"},
    {KEY_KEYPADMINUS,       "KeyPad-"},
    {KEY_KEYPADPERIOD,      "KeyPad."},
    {KEY_KEYPADDIVIDE,      "KeyPad/"},
    {KEY_KEYPAD0,           "KeyPad0"},
    {KEY_KEYPAD1,           "KeyPad1"},
    {KEY_KEYPAD2,           "KeyPad2"},
    {KEY_KEYPAD3,           "KeyPad3"},
    {KEY_KEYPAD4,           "KeyPad4"},
    {KEY_KEYPAD5,           "KeyPad5"},
    {KEY_KEYPAD6,           "KeyPad6"},
    {KEY_KEYPAD7,           "KeyPad7"},
    {KEY_KEYPAD8,           "KeyPad8"},
    {KEY_KEYPAD9,           "KeyPad9"},
    {'0',                   "0"},
    {'1',                   "1"},
    {'2',                   "2"},
    {'3',                   "3"},
    {'4',                   "4"},
    {'5',                   "5"},
    {'6',                   "6"},
    {'7',                   "7"},
    {'8',                   "8"},
    {'9',                   "9"},
    {KEY_MWHEELUP,          "MouseWheelUp"},
    {KEY_MWHEELDOWN,        "MouseWheelDown"},

    // villsa 01052014
#ifdef _USE_XINPUT  // XINPUT
    {BUTTON_DPAD_UP,        "DPadUp"},
    {BUTTON_DPAD_DOWN,      "DPadDown"},
    {BUTTON_DPAD_LEFT,      "DPadLeft"},
    {BUTTON_DPAD_RIGHT,     "DPadRight"},
    {BUTTON_START,          "StartButton"},
    {BUTTON_BACK,           "BackButton"},
    {BUTTON_LEFT_THUMB,     "LeftThumb"},
    {BUTTON_RIGHT_THUMB,    "RightThumb"},
    {BUTTON_LEFT_SHOULDER,  "LeftShoulder"},
    {BUTTON_RIGHT_SHOULDER, "RightShoulder"},
    {BUTTON_A,              "ButtonA"},
    {BUTTON_B,              "ButtonB"},
    {BUTTON_X,              "ButtonX"},
    {BUTTON_Y,              "ButtonY"},
    {BUTTON_LEFT_TRIGGER,   "LeftTrigger"},
    {BUTTON_RIGHT_TRIGGER,  "RightTrigger"},
#endif

    {0,                 NULL}
};

//
// M_GetKeyName
//

int M_GetKeyName(char *buff, int key) {
    keyinfo_t *pkey;

    if(((key >= 'a') && (key <= 'z')) || ((key >= '0') && (key <= '9'))) {
        buff[0] = (char)toupper(key);
        buff[1] = 0;
        return true;
    }
    for(pkey = Keys; pkey->name; pkey++) {
        if(pkey->code == key) {
            dstrcpy(buff, pkey->name);
            return true;
        }
    }
    sprintf(buff, "Key%02x", key);
    return false;
}
