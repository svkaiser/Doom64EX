#include <imp/Image>
#include "private.hh"

namespace {
  device_loader loaders_[] = {
      wad_loader,
      zip_loader,
      rom_loader
  };
}
