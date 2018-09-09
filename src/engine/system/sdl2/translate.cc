#include "sdl2_private.hh"
#include "common/doomdef.h"

namespace priv = imp::sdl2_private;

//
// translate_scancode
//
int priv::translate_scancode(const SDL_Scancode key) {
    switch(key) {
    case SDL_SCANCODE_LEFT: return KEY_LEFTARROW;
    case SDL_SCANCODE_RIGHT: return KEY_RIGHTARROW;
    case SDL_SCANCODE_DOWN: return KEY_DOWNARROW;
    case SDL_SCANCODE_UP: return KEY_UPARROW;
    case SDL_SCANCODE_ESCAPE: return KEY_ESCAPE;
    case SDL_SCANCODE_RETURN: return KEY_ENTER;
    case SDL_SCANCODE_TAB: return KEY_TAB;
    case SDL_SCANCODE_F1: return KEY_F1;
    case SDL_SCANCODE_F2: return KEY_F2;
    case SDL_SCANCODE_F3: return KEY_F3;
    case SDL_SCANCODE_F4: return KEY_F4;
    case SDL_SCANCODE_F5: return KEY_F5;
    case SDL_SCANCODE_F6: return KEY_F6;
    case SDL_SCANCODE_F7: return KEY_F7;
    case SDL_SCANCODE_F8: return KEY_F8;
    case SDL_SCANCODE_F9: return KEY_F9;
    case SDL_SCANCODE_F10: return KEY_F10;
    case SDL_SCANCODE_F11: return KEY_F11;
    case SDL_SCANCODE_F12: return KEY_F12;
    case SDL_SCANCODE_BACKSPACE: return KEY_BACKSPACE;
    case SDL_SCANCODE_DELETE: return KEY_DEL;
    case SDL_SCANCODE_INSERT: return KEY_INSERT;
    case SDL_SCANCODE_PAGEUP: return KEY_PAGEUP;
    case SDL_SCANCODE_PAGEDOWN: return KEY_PAGEDOWN;
    case SDL_SCANCODE_HOME: return KEY_HOME;
    case SDL_SCANCODE_END: return KEY_END;
    case SDL_SCANCODE_PAUSE: return KEY_PAUSE;
    case SDL_SCANCODE_EQUALS: return KEY_EQUALS;
    case SDL_SCANCODE_MINUS: return KEY_MINUS;
    case SDL_SCANCODE_KP_0: return KEY_KEYPAD0;
    case SDL_SCANCODE_KP_1: return KEY_KEYPAD1;
    case SDL_SCANCODE_KP_2: return KEY_KEYPAD2;
    case SDL_SCANCODE_KP_3: return KEY_KEYPAD3;
    case SDL_SCANCODE_KP_4: return KEY_KEYPAD4;
    case SDL_SCANCODE_KP_5: return KEY_KEYPAD5;
    case SDL_SCANCODE_KP_6: return KEY_KEYPAD6;
    case SDL_SCANCODE_KP_7: return KEY_KEYPAD7;
    case SDL_SCANCODE_KP_8: return KEY_KEYPAD8;
    case SDL_SCANCODE_KP_9: return KEY_KEYPAD9;
    case SDL_SCANCODE_KP_PLUS: return KEY_KEYPADPLUS;
    case SDL_SCANCODE_KP_MINUS: return KEY_KEYPADMINUS;
    case SDL_SCANCODE_KP_DIVIDE: return KEY_KEYPADDIVIDE;
    case SDL_SCANCODE_KP_MULTIPLY: return KEY_KEYPADMULTIPLY;
    case SDL_SCANCODE_KP_ENTER: return KEY_KEYPADENTER;
    case SDL_SCANCODE_KP_PERIOD: return KEY_KEYPADPERIOD;

    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_RSHIFT: return KEY_RSHIFT;

    case SDL_SCANCODE_LCTRL:
    case SDL_SCANCODE_RCTRL: return KEY_RCTRL;

    case SDL_SCANCODE_LALT:
    case SDL_SCANCODE_RALT: return KEY_RALT;

    case SDL_SCANCODE_CAPSLOCK: return KEY_CAPS;
    case SDL_SCANCODE_GRAVE: return KEY_GRAVE;

    default: return key;
    }
}

//
// translate_sdlk
//
int priv::translate_sdlk(const int key) {
    switch(key) {
    case SDLK_LEFT: return KEY_LEFTARROW;
    case SDLK_RIGHT: return KEY_RIGHTARROW;
    case SDLK_DOWN: return KEY_DOWNARROW;
    case SDLK_UP: return KEY_UPARROW;
    case SDLK_ESCAPE: return KEY_ESCAPE;
    case SDLK_RETURN: return KEY_ENTER;
    case SDLK_TAB: return KEY_TAB;
    case SDLK_F1: return KEY_F1;
    case SDLK_F2: return KEY_F2;
    case SDLK_F3: return KEY_F3;
    case SDLK_F4: return KEY_F4;
    case SDLK_F5: return KEY_F5;
    case SDLK_F6: return KEY_F6;
    case SDLK_F7: return KEY_F7;
    case SDLK_F8: return KEY_F8;
    case SDLK_F9: return KEY_F9;
    case SDLK_F10: return KEY_F10;
    case SDLK_F11: return KEY_F11;
    case SDLK_F12: return KEY_F12;
    case SDLK_BACKSPACE: return KEY_BACKSPACE;
    case SDLK_DELETE: return KEY_DEL;
    case SDLK_INSERT: return KEY_INSERT;
    case SDLK_PAGEUP: return KEY_PAGEUP;
    case SDLK_PAGEDOWN: return KEY_PAGEDOWN;
    case SDLK_HOME: return KEY_HOME;
    case SDLK_END: return KEY_END;
    case SDLK_PAUSE: return KEY_PAUSE;
    case SDLK_EQUALS: return KEY_EQUALS;
    case SDLK_MINUS: return KEY_MINUS;
    case SDLK_KP_0: return KEY_KEYPAD0;
    case SDLK_KP_1: return KEY_KEYPAD1;
    case SDLK_KP_2: return KEY_KEYPAD2;
    case SDLK_KP_3: return KEY_KEYPAD3;
    case SDLK_KP_4: return KEY_KEYPAD4;
    case SDLK_KP_5: return KEY_KEYPAD5;
    case SDLK_KP_6: return KEY_KEYPAD6;
    case SDLK_KP_7: return KEY_KEYPAD7;
    case SDLK_KP_8: return KEY_KEYPAD8;
    case SDLK_KP_9: return KEY_KEYPAD9;
    case SDLK_KP_PLUS: return KEY_KEYPADPLUS;
    case SDLK_KP_MINUS: return KEY_KEYPADMINUS;
    case SDLK_KP_DIVIDE: return KEY_KEYPADDIVIDE;
    case SDLK_KP_MULTIPLY: return KEY_KEYPADMULTIPLY;
    case SDLK_KP_ENTER: return KEY_KEYPADENTER;
    case SDLK_KP_PERIOD: return KEY_KEYPADPERIOD;

    case SDLK_LSHIFT:
    case SDLK_RSHIFT: return KEY_RSHIFT;

    case SDLK_LCTRL:
    case SDLK_RCTRL: return KEY_RCTRL;

    case SDLK_LALT:
    case SDLK_RALT: return KEY_RALT;

    case SDLK_CAPSLOCK: return KEY_CAPS;

    default: return key;
    }
}

//
// translate_controller
//
int priv::translate_controller(int state)
{
    switch (state) {
    case SDL_CONTROLLER_BUTTON_A: return JOY_A;
    case SDL_CONTROLLER_BUTTON_B: return JOY_B;
    case SDL_CONTROLLER_BUTTON_X: return JOY_X;
    case SDL_CONTROLLER_BUTTON_Y: return JOY_Y;

    case SDL_CONTROLLER_BUTTON_BACK: return JOY_BACK;
    case SDL_CONTROLLER_BUTTON_GUIDE: return JOY_GUIDE;
    case SDL_CONTROLLER_BUTTON_START: return JOY_START;
    case SDL_CONTROLLER_BUTTON_LEFTSTICK: return JOY_LSTICK;
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return JOY_RSTICK;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return JOY_LSHOULDER;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return JOY_RSHOULDER;

    case SDL_CONTROLLER_BUTTON_DPAD_UP: return JOY_DPAD_UP;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return JOY_DPAD_DOWN;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return JOY_DPAD_LEFT;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return JOY_DPAD_RIGHT;

    default: return JOY_INVALID;
    }
}

//
// translate_mouse
//
int priv::translate_mouse(int state) {
    return 0
        | (state & SDL_BUTTON(SDL_BUTTON_LEFT)      ? 1 : 0)
        | (state & SDL_BUTTON(SDL_BUTTON_MIDDLE)    ? 2 : 0)
        | (state & SDL_BUTTON(SDL_BUTTON_RIGHT)     ? 4 : 0);
}
