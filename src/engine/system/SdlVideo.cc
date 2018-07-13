#include "SDL.h"
#include <imp/Video>
#include <core/cvar.hh>
#include <config.hh>
#include <doomdef.h>
#include <doom_main/d_main.h>
#include <SDL_video.h>
#include <common/doomstat.h>
#include <renderer/r_main.h>

namespace {
  // Exclusive fullscreen by default on Windows only.
#ifdef _WIN32
  constexpr auto fullscreen_default = 0;
  constexpr auto fullscreen_enum = Fullscreen::exclusive;
#else
  constexpr auto fullscreen_default = 2;
  constexpr auto fullscreen_enum = Fullscreen::noborder;
#endif

  SDL_GameController *g_controller {};

  int translate_scancode_(const SDL_Scancode key) {
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

  int translate_sdlk_(const int key) {
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

  int translate_controller_(int state)
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

  int translate_mouse_(int state) {
      return 0
          | (state & SDL_BUTTON(SDL_BUTTON_LEFT)      ? 1 : 0)
          | (state & SDL_BUTTON(SDL_BUTTON_MIDDLE)    ? 2 : 0)
          | (state & SDL_BUTTON(SDL_BUTTON_RIGHT)     ? 4 : 0);
  }
}

// Doom globals
int mouse_x {};
int mouse_y {};

/* Graphics */
IntCvar v_width { "v_Width", "Video width (-1: desktop width)", -1 };
IntCvar v_height { "v_Height", "Video height (-1: desktop height)", -1 };
IntCvar v_depthsize { "v_DepthSize", "Depth buffer fragment size", 24 };
IntCvar v_buffersize { "v_BufferSize", "Framebuffer fragment size", 32 };
BoolCvar v_vsync { "v_VSync", "Vertical sync", true };
IntCvar v_windowed { "v_Windowed", "Window mode", fullscreen_default };

/* Mouse Input */
FloatCvar v_msensitivityx { "v_MSensitivityX", "Mouse sensitivity", 5.0f };
FloatCvar v_msensitivityy { "v_MSensitivityY", "Mouse sensitivity", 5.0f };
FloatCvar v_macceleration { "v_MAcceleration", "Mouse acceleration", 0.0f };
BoolCvar v_mlook { "v_MLook", "Mouse-look", true };
BoolCvar v_mlookinvert { "v_MLookInvert", "Invert Y-Axis", false };
BoolCvar v_yaxismove { "v_YAxisMove", "Move with the mouse", false };

class SdlVideo : public IVideo {
    SDL_Window* sdl_window_ {};
    SDL_GLContext sdl_glcontext_ {};
    OpenGLVer opengl_;
    bool has_focus_ { false };
    bool has_mouse_ { false };
    Vector<VideoMode> modes_ {};
    Uint32 lastmbtn_ {};

    void init_gl_() {
        switch (opengl_) {
        case OpenGLVer::gl14:
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
            break;

        case OpenGLVer::gl33:
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            break;
        }

        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    }

    VideoMode sanitize_mode_(const VideoMode &mode) {
        VideoMode copy = mode;

        copy.width = std::max(copy.width, 320);
        copy.height = std::max(copy.height, 240);

        if (copy.depth_size != 8 && copy.depth_size != 16 && copy.depth_size != 24)
            copy.depth_size = 24;

        if (copy.buffer_size != 8 && copy.buffer_size != 16 && copy.buffer_size != 24 && copy.buffer_size != 32)
            copy.buffer_size = 32;

        return copy;
    }

    void init_modes_()
    {
        SDL_DisplayMode prev_mode, mode;
        int index = 0;
        int num_displays = SDL_GetNumVideoDisplays();
        log::debug("Available video modes:");
        for (int i = 0; i < num_displays; ++i) {
            int num_modes = SDL_GetNumDisplayModes(i);
            for (int j = 0; j < num_modes; ++j) {
                SDL_GetDisplayMode(i, j, &mode);

                if (j > 0 && prev_mode.w == mode.w && prev_mode.h == mode.h)
                    continue;

                log::debug("{}: {}x{} on display #{}", index++, mode.w, mode.h, i);
                modes_.push_back({ mode.w, mode.h });
                prev_mode = mode;
            }
        }
    }

    int mouse_accel_(int val) {
        if (*v_macceleration == 0.0f)
            return val;

        if (val < 0)
            return -mouse_accel_(-val);

        float factor = *v_macceleration / 200.0f + 1.0f;
        return static_cast<int>(pow(static_cast<float>(val), factor));
    }

public:
    SdlVideo(OpenGLVer opengl):
        opengl_(opengl)
    {
        SDL_Init(SDL_INIT_EVERYTHING);
        SDL_ShowCursor(SDL_FALSE);
        init_modes_();

        SDL_DisplayMode desktop_mode;
        SDL_GetDesktopDisplayMode(0, &desktop_mode);

        VideoMode mode = {
            *v_width,
            *v_height,
            *v_buffersize,
            *v_depthsize,
            Fullscreen::none,
            *v_vsync
        };

        if (mode.width < 0)
            mode.width = desktop_mode.w;
        if (mode.height < 0)
            mode.height = desktop_mode.h;

        switch (*v_windowed) {
        case 0:
            mode.fullscreen = Fullscreen::exclusive;
            break;

        case 2:
            mode.fullscreen = Fullscreen::noborder;
            break;

        default:
            break;
        }

        set_mode(mode);
    }

    ~SdlVideo()
    {
        if (sdl_glcontext_) {
            SDL_GL_DeleteContext(sdl_glcontext_);
        }
        if (sdl_window_) {
            SDL_DestroyWindow(sdl_window_);
        }
        SDL_Quit();
    }

    void set_mode(const VideoMode& mode) override
    {
        VideoMode copy = sanitize_mode_(mode);

        SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, copy.buffer_size);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, copy.depth_size);
        SDL_GL_SetSwapInterval(copy.vsync ? 1 : 0);

        if (!sdl_window_) {
            // Prepare SDL's GL attributes
            init_gl_();

            uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS;
            switch (copy.fullscreen) {
            case Fullscreen::none:
                break;

            case Fullscreen::noborder:
                flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
                break;

            case Fullscreen::exclusive:
                flags |= SDL_WINDOW_FULLSCREEN;
                break;
            }

            auto title = fmt::format("{} {} - SDL2, OpenGL 1.4", config::name, config::version_full);
            sdl_window_ = SDL_CreateWindow(title.c_str(),
                                           SDL_WINDOWPOS_CENTERED,
                                           SDL_WINDOWPOS_CENTERED,
                                           copy.width,
                                           copy.height,
                                           flags);

            if (!sdl_window_)
                throw video_error { fmt::format("Couldn't create window: {}", SDL_GetError()) };

            sdl_glcontext_ = SDL_GL_CreateContext(sdl_window_);

            if (!sdl_glcontext_)
                throw video_error { fmt::format("Couldn't create OpenGL Context: {}", SDL_GetError()) };
        } else {
            SDL_SetWindowSize(sdl_window_, copy.width, copy.height);

            switch (copy.fullscreen) {
            case Fullscreen::none:
                SDL_SetWindowFullscreen(sdl_window_, 0);
                break;

            case Fullscreen::noborder:
                SDL_SetWindowFullscreen(sdl_window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
                break;

            case Fullscreen::exclusive:
                SDL_SetWindowFullscreen(sdl_window_, SDL_WINDOW_FULLSCREEN);
                break;
            }

            SDL_GetWindowSize(sdl_window_, &video_width, &video_height);
            video_ratio = static_cast<float>(video_width) / video_height;
            glViewport(0, 0, video_width, video_height);
            GL_CalcViewSize();
            R_SetViewMatrix();
        }

        v_width = copy.width;
        v_height = copy.height;
        v_depthsize = copy.depth_size;
        v_buffersize = copy.buffer_size;
        v_vsync = copy.vsync;

        switch (copy.fullscreen) {
        case Fullscreen::none:
            v_windowed = 1;
            break;

        case Fullscreen::noborder:
            v_windowed = 2;
            break;

        case Fullscreen::exclusive:
            v_windowed = 0;
            break;
        }
    }

    VideoMode current_mode() override
    {
        VideoMode mode;
        SDL_GetWindowSize(sdl_window_, &mode.width, &mode.height);
        SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE, &mode.buffer_size);
        SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &mode.depth_size);

        uint32 flags = SDL_GetWindowFlags(sdl_window_);
        if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
            mode.fullscreen = Fullscreen::noborder;
        } else if (flags & SDL_WINDOW_FULLSCREEN) {
            mode.fullscreen = Fullscreen::exclusive;
        } else {
            mode.fullscreen = Fullscreen::none;
        }

        mode.vsync = SDL_GL_GetSwapInterval() == SDL_TRUE;

        return mode;
    }

    ArrayView<VideoMode> modes() override
    {
        return modes_;
    }

    void swap_window() override
    {
        SDL_GL_SwapWindow(sdl_window_);
    }

    void grab(bool g) override
    {
        if (g) {
            SDL_ShowCursor(SDL_FALSE);
            SDL_SetRelativeMouseMode(SDL_TRUE);
            SDL_SetWindowGrab(sdl_window_, SDL_TRUE);
        } else {
            SDL_SetRelativeMouseMode(SDL_FALSE);
            SDL_SetWindowGrab(sdl_window_, SDL_FALSE);
            SDL_ShowCursor(SDL_FALSE);
        }
    }

    void poll_events() override
    {
        event_t doom {};
        SDL_Event e;

        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_KEYDOWN: {
                if (e.key.repeat > 0)
                    break;

                auto key = e.key.keysym;
                if (key.scancode == SDL_SCANCODE_RETURN && key.mod == KMOD_RALT) {
                    auto mode = current_mode();
                    if (mode.fullscreen != fullscreen_enum)
                        mode.fullscreen = fullscreen_enum;
                    else mode.fullscreen = Fullscreen::none;
                    set_mode(mode);
                    break;
                }

                doom.type = ev_keydown;
                doom.data1 = translate_sdlk_(e.key.keysym.sym);
                doom.data2 = translate_scancode_(e.key.keysym.scancode);
                D_PostEvent(&doom);
                break;
            }

            case SDL_KEYUP:
                doom.type = ev_keyup;
                doom.data1 = translate_sdlk_(e.key.keysym.sym);
                doom.data2 = translate_scancode_(e.key.keysym.scancode);
                D_PostEvent(&doom);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                if (!has_focus_)
                    break;

                doom.type = (e.type == SDL_MOUSEBUTTONUP) ? ev_mouseup : ev_mousedown;
                doom.data1 = translate_mouse_(SDL_GetMouseState(nullptr, nullptr));
                D_PostEvent(&doom);
                break;

            case SDL_MOUSEWHEEL:
                if (e.wheel.y > 0) {
                    doom.type = ev_keydown;
                    doom.data1 = KEY_MWHEELUP;
                } else if (e.wheel.y < 0) {
                    doom.type = ev_keydown;
                    doom.data1 = KEY_MWHEELDOWN;
                } else break;

                D_PostEvent(&doom);
                break;

            // case SDL_CONTROLLERDEVICEADDED:
            //     log::info("Controller added: {}", SDL_GameControllerNameForIndex(e.cdevice.which));
            //     g_controller = SDL_GameControllerOpen(e.cdevice.which);
            //     break;

            // case SDL_CONTROLLERDEVICEREMOVED:
            //     g_controller = nullptr;
            //     break;

            // case SDL_CONTROLLERBUTTONDOWN:
            // case SDL_CONTROLLERBUTTONUP:
            //     if (!has_focus_)
            //         break;

            //     doom.type = (e.type == SDL_CONTROLLERBUTTONUP) ? ev_conup : ev_condown;
            //     doom.data1 = translate_controller_(e.cbutton.which);
            //     D_PostEvent(&doom);
            //     break;

            case SDL_WINDOWEVENT:
                switch (e.window.event) {
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    has_focus_ = true;
                    break;

                case SDL_WINDOWEVENT_FOCUS_LOST:
                    has_focus_ = false;
                    break;

                case SDL_WINDOWEVENT_ENTER:
                    has_mouse_ = true;
                    break;

                case SDL_WINDOWEVENT_LEAVE:
                    has_mouse_ = false;
                    break;

                default:
                    break;
                }
                break;

            case SDL_QUIT:
                I_Quit();
                return;

            default:
                break;
            }
        }

        int x, y;

        // if (g_controller) {
        //     event_t ev {};

        //     x = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_LEFTX);
        //     y = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_LEFTY);

        //     ev.type = ev_conleft;
        //     ev.data1 = 0;
        //     ev.data2 = x << 5;
        //     ev.data3 = (-y) << 5;
        //     D_PostEvent(&ev);

        //     x = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_RIGHTX);
        //     y = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_RIGHTY);

        //     ev.type = ev_conright;
        //     ev.data1 = 0;
        //     ev.data2 = x << 5;
        //     ev.data3 = (-y) << 5;
        //     D_PostEvent(&ev);
        // }

        SDL_GetRelativeMouseState(&x, &y);
        auto btn = SDL_GetMouseState(&mouse_x, &mouse_y);
        if (x != 0 || y != 0 || btn || (lastmbtn_ != btn)) {
            event_t ev {};
            ev.type = ev_mouse;
            ev.data1 = translate_mouse_(btn);
            ev.data2 = x << 5;
            ev.data3 = (-y) << 5;
            D_PostEvent(&ev);
            lastmbtn_ = btn;
        }
    }
};

void init_video_sdl()
{
    Video = new SdlVideo { OpenGLVer::gl14 };
}
