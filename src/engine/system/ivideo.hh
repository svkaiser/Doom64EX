// -*- C++ -*-
#ifndef __IMP_VIDEO__38697988
#define __IMP_VIDEO__38697988

#include "prelude.hh"

namespace imp {
  enum struct Fullscreen {
      none, // Windowed
      noborder, // Noborder fullscreen window
      exclusive // Exclusive fullscreen
  };

  enum struct OpenGLVer {
      gl14,
      gl33
  };

  struct VideoMode {
      int width;
      int height;
      int buffer_size { 32 };
      int depth_size { 24 };
      Fullscreen fullscreen { Fullscreen::none };
      bool vsync { false };
  };

  struct IVideo {
      virtual ~IVideo() {}

      /**
       * Tries to set video mode. If successful, should also set relevant cvars.
       */
      virtual void set_mode(const VideoMode&) = 0;

      /**
       * Enable/disable vsync
       */
      virtual void set_vsync(bool should_sync) = 0;

      /**
       * @return Current video mode
       */
      virtual VideoMode current_mode() = 0;

      /**
       * List available (fullscreen) video modes
       * @return A list of video modes
       */
      virtual ArrayView<VideoMode> modes() = 0;

      /**
       * Grab mouse and keyboard
       * @param should_grab
       */
      virtual void grab(bool should_grab) = 0;

      /**
       * Called at the beginning of a new frame.
       */
      virtual void begin_frame() = 0;

      /**
       * Called at the end of a frame. Should swap window etc.
       */
      virtual void end_frame() = 0;

      bool is_windowed()
      { return current_mode().fullscreen == Fullscreen::none; }
  };

  extern IVideo* Video;
}

#endif //__IMP_VIDEO__38697988
