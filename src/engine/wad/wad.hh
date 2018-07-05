#ifndef __WAD__60795258
#define __WAD__60795258

#include "idevice.hh"
#include "lump.hh"

namespace imp {
  namespace wad {
    void init();

    /*!
     * Add a device loader callback
     * @param device_loader
     */
    bool add_device_loader(IDeviceLoader& device_loader);

    /*!
     *
     * @param device
     */
    bool add_device(IDevicePtr device);

    bool add_device(StringView path);

    void merge();

    Optional<Lump> open(Section section, StringView path);

    /*!
     * Open a Lump by its section index
     * @param section
     * @param path
     * @return
     */
    Optional<Lump> open(Section section, size_t index);

    Optional<Lump> open(size_t index);

    inline Optional<Lump> open(StringView path)
    { return open(Section::normal, path); }

    inline bool exists(Section section, StringView path)
    { return static_cast<bool>(open(section, path)); }

    inline bool exists(StringView path)
    { return exists(Section::normal, path); }

    ArrayView<ILumpPtr> list_section(Section);
  }
}

#endif //__WAD__60795258
