#ifndef __IDEVICE__11091036
#define __IDEVICE__11091036

#include "ilump.hh"

namespace imp {
  namespace wad {
    class IDevice {
    public:
        virtual ~IDevice() {}

        virtual Vector<ILumpPtr> read_all() = 0;
    };

    using IDevicePtr = UniquePtr<IDevice>;
    using IDeviceLoader = IDevicePtr (StringView);
  }
}


#endif //__IDEVICE__11091036
