#include <platform/app.hh>
#include "wad.hh"
#include "wad_loaders.hh"

void wad::init()
{
    // Add device loaders
    wad::add_device_loader(zip_loader);
    wad::add_device_loader(doom_loader);

    // Find and add the Doom 64 IWAD
    if (auto d64_data_path = app::find_data_file("doom64.wad")) {
        wad::add_device(*d64_data_path);
    } else if (auto n64_data_path = app::find_data_file("doom64.rom")) {
        // TODO
    } else {
        fatal("Couldn't find 'doom64.rom'");
    }

    // Find and add 'doom64ex.pk3'
    if (auto engine_data_path = app::find_data_file("doom64ex.pk3")) {
        wad::add_device(*engine_data_path);
    } else {
        fatal("Couldn't find 'doom64ex.pk3'");
    }

    wad::merge();
}
