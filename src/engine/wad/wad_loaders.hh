#ifndef DOOM64EX_WAD_LOADERS_HH
#define DOOM64EX_WAD_LOADERS_HH

#include "device.hh"

namespace imp {
  namespace wad {
    DevicePtr zip_loader(StringView);
    DevicePtr doom_loader(StringView);
  }
}

#endif //DOOM64EX_WAD_LOADERS_HH
