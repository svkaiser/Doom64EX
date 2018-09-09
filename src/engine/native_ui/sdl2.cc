#include "SDL.h"
#include "native_ui/native_ui.hh"

void native_ui::error(const std::string& message)
{
    SDL_ShowSimpleMessageBox(0, "Doom64EX Error", message.c_str(), nullptr);
}

void (*__imp_error)(const std::string&) = native_ui::error;
