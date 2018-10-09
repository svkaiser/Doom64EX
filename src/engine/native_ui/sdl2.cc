#include "SDL.h"
#include "native_ui.hh"

/* Initialise Native UI to SDL2 at startup by default */
UniquePtr<NativeUI> imp::g_native_ui = std::make_unique<NativeUI>();

NativeUI::NativeUI()
{
    /* no-op */
}

NativeUI::~NativeUI()
{
    /* no-op */
}

void NativeUI::console_show(bool)
{
    /* no-op */
}

void NativeUI::console_add_line(StringView)
{
    /* no-op */
}

void NativeUI::error(const std::string &line)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Doom64EX Fatal Error", line.c_str(), nullptr);
}

Optional<String> NativeUI::rom_select()
{
    return nullopt;
}
