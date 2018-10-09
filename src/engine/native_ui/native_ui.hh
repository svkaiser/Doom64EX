// -*- mode: c++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2017 Zohar Malamant
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

#ifndef __IMP_NATIVEUI__92059425
#define __IMP_NATIVEUI__92059425

#include "prelude.hh"

namespace imp {
  class NativeUI {
  public:
      NativeUI();

      virtual ~NativeUI();

      virtual void console_show(bool do_show);

      virtual void console_add_line(StringView message);

      virtual void error(const std::string& line);

      /**
       * Show the user a file dialog to find a valid ROM file
       *
       * \returns a
       */
      virtual Optional<String> rom_select();
  };

  extern UniquePtr<NativeUI> g_native_ui;
}

#endif //__IMP_NATIVEUI__92059425
