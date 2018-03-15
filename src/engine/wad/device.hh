#ifndef __DEVICE__11091036
#define __DEVICE__11091036

#include "ilump.hh"

namespace imp {
  namespace wad {
    class Device {
    public:
        virtual ~Device() {}

        virtual Vector<ILumpPtr> read_all() = 0;
    };

    using DevicePtr = UniquePtr<Device>;

    using DeviceLoader = DevicePtr (StringView);
  }
}


#endif //__DEVICE__11091036
