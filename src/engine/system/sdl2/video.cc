#include "system/ivideo.hh"
#include "core/cvar.hh"
#include "config.hh"
#include "doomdef.h"
#include "doom_main/d_main.h"
#include "common/doomstat.h"
#include "renderer/r_main.h"

#include "sdl2_private.hh"

namespace priv = imp::sdl2_private;

namespace {
  // Exclusive fullscreen by default on Windows only.
#ifdef _WIN32
  constexpr auto fullscreen_default = 0;
  constexpr auto fullscreen_enum = Fullscreen::exclusive;
#else
  constexpr auto fullscreen_default = 2;
  constexpr auto fullscreen_enum = Fullscreen::noborder;
#endif

  SDL_GameController *s_controller {};

  /**
   * Convert mode to a valid mode.
   */
  VideoMode s_sanitize_mode(const VideoMode &mode) {
      VideoMode copy = mode;

      copy.width = std::max(copy.width, 320);
      copy.height = std::max(copy.height, 240);

      if (copy.depth_size != 8 && copy.depth_size != 16 && copy.depth_size != 24)
          copy.depth_size = 24;

      if (copy.buffer_size != 8 && copy.buffer_size != 16 && copy.buffer_size != 24 && copy.buffer_size != 32)
          copy.buffer_size = 32;

      return copy;
  }

  class SdlVideo : public IVideo {
      SDL_Window* m_window {};
      SDL_GLContext m_glcontext {};
      OpenGLVer m_opengl;
      bool m_has_focus { false };
      bool m_has_mouse { false };
      Vector<VideoMode> m_modes {};
      Uint32 m_lastmbtn {};

      void m_init_gl();
      void m_init_modes();
      int m_mouse_accel(int val);

  public:
      SdlVideo(OpenGLVer ver);
      ~SdlVideo();

      void set_mode(const VideoMode& mode) override;
      VideoMode current_mode() override;

      void grab(bool should_grab) override;
      void begin_frame() override;
      void end_frame() override;

      ArrayView<VideoMode> modes() override
      { return m_modes; }
  };
}

// Doom globals
int mouse_x {};
int mouse_y {};

/* Graphics */
cvar::IntVar v_width = -1;
cvar::IntVar v_height = -1;
cvar::IntVar v_depthsize = 24;
cvar::IntVar v_buffersize = 32;
cvar::BoolVar v_vsync = true;
cvar::IntVar v_windowed = fullscreen_default;

/* Mouse Input */
cvar::FloatVar v_msensitivityx = 5.0f;
cvar::FloatVar v_msensitivityy = 5.0f;
cvar::FloatVar v_macceleration = 0.0f;
cvar::BoolVar v_mlook = true;
cvar::BoolVar v_mlookinvert = false;
cvar::BoolVar v_yaxismove = false;

//
// SdlVideo::m_init_gl
//
void SdlVideo::m_init_gl() {
    switch (m_opengl) {
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

//
// SdlVideo::m_init_modes
//
void SdlVideo::m_init_modes()
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
            m_modes.push_back({ mode.w, mode.h });
            prev_mode = mode;
        }
    }
}

//
// SdlVideo::m_mouse_accel
//
int SdlVideo::m_mouse_accel(int val) {
    if (*v_macceleration == 0.0f)
        return val;

    if (val < 0)
        return -m_mouse_accel(-val);

    float factor = *v_macceleration / 200.0f + 1.0f;
    return static_cast<int>(pow(static_cast<float>(val), factor));
}

//
// SdlVideo::SdlVideo
//
SdlVideo::SdlVideo(OpenGLVer opengl):
    m_opengl(opengl)
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_ShowCursor(SDL_FALSE);
    m_init_modes();

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

//
// SdlVideo::~SdlVideo
//
SdlVideo::~SdlVideo()
{
    if (m_glcontext) {
        SDL_GL_DeleteContext(m_glcontext);
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
    }
    SDL_Quit();
}

//
// SdlVideo::set_mode
//
void SdlVideo::set_mode(const VideoMode& mode)
{
    VideoMode copy = s_sanitize_mode(mode);

    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, copy.buffer_size);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, copy.depth_size);
    SDL_GL_SetSwapInterval(copy.vsync ? 1 : 0);

    if (!m_window) {
        // Prepare SDL's GL attributes
        m_init_gl();

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
        m_window = SDL_CreateWindow(title.c_str(),
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       copy.width,
                                       copy.height,
                                       flags);

        if (!m_window)
            throw std::runtime_error  { fmt::format("Couldn't create window: {}", SDL_GetError()) };

        m_glcontext = SDL_GL_CreateContext(m_window);

        if (!m_glcontext)
            throw std::runtime_error { fmt::format("Couldn't create OpenGL Context: {}", SDL_GetError()) };
    } else {
        SDL_DisplayMode target {}, closest {};
        auto display_id = SDL_GetWindowDisplayIndex(m_window);
        switch (copy.fullscreen) {
        case Fullscreen::none:
            SDL_SetWindowFullscreen(m_window, 0);

            SDL_SetWindowSize(m_window, copy.width, copy.height);

            video_width = copy.width;
            video_height = copy.height;
            break;

        case Fullscreen::noborder:
            SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN_DESKTOP);

            /* this mode is always using the desktop display resolution */
            SDL_GetDesktopDisplayMode(display_id, &target);

            video_width = target.w;
            video_height = target.h;
            break;

        case Fullscreen::exclusive:
            SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN);

            // Find an appropriate display mode
            target.w = copy.width;
            target.h = copy.height;

            SDL_GetClosestDisplayMode(display_id, &target, &closest);

            SDL_SetWindowDisplayMode(m_window, &closest);

            video_width = closest.w;
            video_height = closest.h;
            break;
        }

        video_ratio = static_cast<float>(video_width) / video_height;
        ViewWidth = video_width;
        ViewHeight = video_height;
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

//
// SdlVideo::current_mode
//
VideoMode SdlVideo::current_mode()
{
    VideoMode mode;
    SDL_GetWindowSize(m_window, &mode.width, &mode.height);
    SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE, &mode.buffer_size);
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &mode.depth_size);

    uint32 flags = SDL_GetWindowFlags(m_window);
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

//
// SdlVideo::grab
//
void SdlVideo::grab(bool should_grab)
{
    if (should_grab) {
        SDL_ShowCursor(SDL_FALSE);
        SDL_SetRelativeMouseMode(SDL_TRUE);
        SDL_SetWindowGrab(m_window, SDL_TRUE);
    } else {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_SetWindowGrab(m_window, SDL_FALSE);
        SDL_ShowCursor(SDL_FALSE);
    }
}

//
// SdlVideo::begin_frame
//
void SdlVideo::begin_frame()
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
            doom.data1 = priv::translate_sdlk(e.key.keysym.sym);
            doom.data2 = priv::translate_scancode(e.key.keysym.scancode);
            D_PostEvent(&doom);
            break;
        }

        case SDL_KEYUP:
            doom.type = ev_keyup;
            doom.data1 = priv::translate_sdlk(e.key.keysym.sym);
            doom.data2 = priv::translate_scancode(e.key.keysym.scancode);
            D_PostEvent(&doom);
            break;

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            if (!m_has_focus)
                break;

            doom.type = (e.type == SDL_MOUSEBUTTONUP) ? ev_mouseup : ev_mousedown;
            doom.data1 = priv::translate_mouse(SDL_GetMouseState(nullptr, nullptr));
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
                m_has_focus = true;
                break;

            case SDL_WINDOWEVENT_FOCUS_LOST:
                m_has_focus = false;
                break;

            case SDL_WINDOWEVENT_ENTER:
                m_has_mouse = true;
                break;

            case SDL_WINDOWEVENT_LEAVE:
                m_has_mouse = false;
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
    if (x != 0 || y != 0 || btn || (m_lastmbtn != btn)) {
        event_t ev {};
        ev.type = ev_mouse;
        ev.data1 = priv::translate_mouse(btn);
        ev.data2 = x << 5;
        ev.data3 = (-y) << 5;
        D_PostEvent(&ev);
        m_lastmbtn = btn;
    }
}

//
// SdlVideo::end_frame
//
void SdlVideo::end_frame()
{
    SDL_GL_SwapWindow(m_window);
}

//
// init_video_sdl
//
void init_video_sdl()
{
    cvar::Register()
        /* Video */
        (v_width, "v_Width", "Video width (-1 for auto)")
        (v_height, "v_Height", "Video height (-1 for auto)")
        (v_depthsize, "v_DepthSize", "Depth buffer fragment size")
        (v_buffersize, "v_BufferSize", "Framebuffer fragment size")
        (v_vsync, "v_VSync", "Vertical sync")
        (v_windowed, "v_Windowed", "Window mode")

        /* Mouse Input */
        (v_msensitivityx, "v_MSensitivityX", "Mouse X-axis sensitivity")
        (v_msensitivityy, "v_MSensitivityY", "Mouse Y-axis sensitivity")
        (v_mlook, "v_MLook", "Mouse-look")
        (v_mlookinvert, "v_MLookInvert", "Invert Y-axis")
        (v_yaxismove, "v_YAxisMove", "Move with the mouse");

    Video = new SdlVideo { OpenGLVer::gl14 };
}
