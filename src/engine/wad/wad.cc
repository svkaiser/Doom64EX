#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <platform/app.hh>
#include "native_ui/native_ui.hh"
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

    /* Find and add the Doom 64 IWAD */
    while (!iwad_loaded) {
        if ((path = app::find_data_file("doom64.rom"))) {
            iwad_loaded = wad::add_device(*path);
        }

#ifdef _WIN32
        if (!iwad_loaded) {
            char cd[MAX_PATH];
            GetCurrentDirectory(MAX_PATH, cd);

            auto str = native_ui::rom_select();

            SetCurrentDirectory(cd);

            if (!str) {
                std::exit(0);
            }

            CopyFile(str->c_str(), "doom64.rom", FALSE);
        }
#else
        if (!iwad_loaded) {
            auto str = g_native_ui->rom_select();

            if (!str) {
                log::fatal("Couldn't find 'doom64.rom'");
            }

            auto path = fmt::format("{}/.local/share/doom64ex/doom64.rom", getenv("HOME"));
            int srcfd = ::open(str->c_str(), O_RDONLY, 0);
            int dstfd = ::open(path.c_str(), O_WRONLY | O_CREAT, 0644);

            struct stat srcstat;
            fstat(srcfd, &srcstat);

            sendfile(dstfd, srcfd, 0, srcstat.st_size);

            close(srcfd);
            close(dstfd);
        }
#endif
    }

    // Find and add 'doom64ex.pk3'
    if (auto engine_data_path = app::find_data_file("doom64ex.pk3")) {
        wad::add_device(*engine_data_path);
    } else {
        log::fatal("Couldn't find 'doom64ex.pk3'");
    }

    wad::merge();
}
