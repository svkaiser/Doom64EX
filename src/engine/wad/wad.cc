#include <platform/app.hh>
#include "wad.hh"
#include "wad_loaders.hh"

void wad::init()
{
    Optional<String> path;
    bool iwad_loaded {};

    // Add device loaders
    wad::add_device_loader(zip_loader);
    wad::add_device_loader(doom_loader);
    wad::add_device_loader(rom_loader);

    // Find and add the Doom 64 IWAD
    if (path = app::find_data_file("doom64.rom")) {
        iwad_loaded = wad::add_device(*path);
    }

    if (!iwad_loaded && (path = app::find_data_file("doom64.wad"))) {
        iwad_loaded = wad::add_device(*path);
    }

    if (!iwad_loaded) {
        log::fatal("Couldn't find 'doom64.rom'");
    }

    // Find and add 'doom64ex.pk3'
    if (auto engine_data_path = app::find_data_file("doom64ex.pk3")) {
        wad::add_device(*engine_data_path);
    } else {
        log::fatal("Couldn't find 'doom64ex.pk3'");
    }

    wad::merge();
}
