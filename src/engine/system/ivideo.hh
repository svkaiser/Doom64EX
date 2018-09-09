// -*- C++ -*-
#ifndef __IMP_VIDEO__38697988
#define __IMP_VIDEO__38697988

#include "prelude.hh"

namespace imp {
  struct video_error : std::runtime_error {
      using std::runtime_error::runtime_error;
  };

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
      virtual void set_mode(const VideoMode&) = 0;
      virtual VideoMode current_mode() = 0;
      virtual ArrayView<VideoMode> modes() = 0;
      virtual void swap_window() = 0;
      virtual void grab(bool) = 0;
      virtual void poll_events() = 0;

      bool is_windowed()
      { return current_mode().fullscreen == Fullscreen::none; }
  };

  extern IVideo* Video;
}

#endif //__IMP_VIDEO__38697988
