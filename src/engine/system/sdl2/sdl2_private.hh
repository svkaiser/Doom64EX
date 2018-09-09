#ifndef __SDL2_PRIVATE__48699070
#define __SDL2_PRIVATE__48699070

#include "SDL.h"

namespace imp {
  namespace sdl2_private {
    /**
     * Convert an SDL2 scancode (the physical key pressed) to a Doom key
     */
    int translate_scancode(const SDL_Scancode key);

    /**
     * Convert an SDL2 key (logical, current user keyboard layout-dependent) to
     * a Doom key
     */
    int translate_sdlk(const int key);

    /**
     * Convert an SDL_Gamepad button to a Doom key
     */
    int translate_controller(int state);

    /**
     * Convert a SDL mouse button to a Doom key
     */
    int translate_mouse(int state);
  }
}

#endif //__SDL2_PRIVATE__48699070
