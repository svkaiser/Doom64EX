#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <future>
#include <imp/NativeUI>

namespace {
  Optional<String> s_show()
  {
      char path[260] {};

      /* Set-up and show file dialog */
      OPENFILENAME ofn;
      ZeroMemory(&ofn, sizeof(ofn));
      ofn.lStructSize     = sizeof(ofn);
      ofn.hwndOwner       = nullptr;
      ofn.lpstrFile       = path;
      ofn.nMaxFile        = sizeof(path);
      ofn.lpstrFilter     = "Rom file\0*.z64\0All\0*.*\0";
      ofn.nFilterIndex    = 1;
      ofn.lpstrFileTitle  = nullptr;
      ofn.nMaxFileTitle   = 0;
      ofn.lpstrInitialDir = nullptr;
      ofn.Flags           = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

      if (!GetOpenFileName(&ofn)) {
          return nullopt;
      }

      return String(path);
  }
}

//
// native_ui::rom_select
//

Optional<String> native_ui::rom_select()
{
    return s_show();
}
