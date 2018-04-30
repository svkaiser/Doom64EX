#ifndef __ILUMP__55802067
#define __ILUMP__55802067

#include <prelude.hh>
#include "section.hh"

namespace imp {
  class Image;
  class Palette;

  namespace wad {
    class IDevice;

    class ILump {
        size_t section_index_ {};
        size_t lump_index_ {};
    public:
        virtual ~ILump() {}

        /*!
         * Get the lump name with the form of /^[A-Z0-9_]{1,8}$/
         * @note Name must be the same for the lifetime of the Lump
         * @return Lump name
         */
        virtual String name() const = 0;

        /*!
         * @note Section must not mutate
         * @return Section type
         */
        virtual Section section() const = 0;

        /*!
         * Get the real file name of the lump.
         * @return File name
         */
        virtual String real_name() const = 0;

        /*!
         * Get the device object that owns this lump.
         * @return Reference to device
         */
        virtual IDevice& device() = 0;

        /*!
         * Create a stream for reading. Must create a new stream object every time this function is called.
         * @return Owned stream pointer
         */
        virtual UniquePtr<std::istream> stream() = 0;

        /*!
         * Interpret the lump as raw bytes.
         * @return The entire contents of the lump as a byte string
         */
        virtual String read_bytes();

        /*!
         * Interpret the lump as raw bytes, C Compatibility mode
         * @note Caller must free
         * @return The entire contents of the lump as a new char array
         */
        virtual char* read_bytes_ccompat(size_t* size_out);

        /*!
         * Interpret the lump as an image.
         * Can be overriden by device to support custom image types
         * @return An optional image object
         */
        virtual Optional<Image> read_image();

        /*!
         * Interpret the lump as a palette.
         * Can be overriden by device to support custom palette types
         * @return An optional palette object
         */
        virtual Optional<Palette> read_palette();

        /*!
         * @return Index in the section
         */
        size_t section_index() const
        { return section_index_; }

        /*!
         * Set index section
         * @param new_index
         */
        void set_section_index(size_t new_index)
        { section_index_ = new_index; }

        size_t lump_index() const
        { return lump_index_; }

        void set_lump_index(size_t new_index)
        { lump_index_ = new_index; }
    };

    /*!
     * Comparator (Mostly for sorting). Compares by name
     */
    inline bool operator<(const ILump& a, const ILump& b)
    { return a.name() < b.name(); }

    using ILumpPtr = UniquePtr<ILump>;
  }
}

#endif //__ILUMP__55802067
