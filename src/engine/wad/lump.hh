#ifndef __LUMP__77664741
#define __LUMP__77664741

#include <istream>
#include <prelude.hh>

#include "ilump.hh"

namespace imp {
  namespace wad {
    struct stream_error : std::logic_error
    { using std::logic_error::logic_error; };

    class Lump {
        size_t m_offset;
        ILump* m_context {};
        UniquePtr<std::istream> m_stream {};

    public:
        /*!
         * Don't allow creating a lump with no context
         */
        Lump() = delete;

        Lump(Lump&&) = default;
        Lump(const Lump&) = default;

        explicit Lump(size_t offset, ILump& context):
            m_offset(offset),
            m_context(&context) {}

        Lump& operator=(Lump&&) = default;
        Lump& operator=(const Lump&) = default;

        /*!
         * Get the lump name of the form /^[A-Z0-9_]{1,8}$/
         * @return Lump name
         */
        String name() const
        { return m_context->name(); }

        /*!
         * @return Section type
         */
        Section section() const
        { return m_context->section(); }

        /*!
         * Get the real file name of the lump.
         * @return File name
         */
        String real_name() const
        { return m_context->real_name(); }

        /*!
         * @return Section-local index
         */
        size_t section_index() const
        { return m_context->section_index(); }

        /*!
         * @return Global lump index
         */
        size_t lump_index() const
        { return m_context->lump_index(); }

        /*!
         * Get the device object that owns this lump.
         * @return Reference to device
         */
        IDevice& device()
        { return m_context->device(); }

        /*!
         * Get a stream for reading
         * @return stream reference
         */
        std::istream& stream()
        {
            if (m_stream)
                return *m_stream;

            m_stream = std::move(m_context->stream());
            if (!m_stream)
                throw stream_error("Lump::stream()");

            return *m_stream;
        }

        /*!
         * Interpret the lump as raw bytes
         * @return
         */
        String read_bytes()
        { return m_context->read_bytes(); }

        /*!
         * Interpret the lump as raw bytes, C Compatibility mode
         * @note Caller must free
         * @param size_out Write lump size to this variable
         * @return The entire contents of the lump as a new char array
         */
        template <class IntT>
        char* read_bytes_ccompat(IntT& size_out)
        {
            static_assert(std::is_integral<IntT>::value, "Must be a pointer to an integral type");
            size_t conv_size_out;
            char* retval = m_context->read_bytes_ccompat(&conv_size_out);
            size_out = static_cast<IntT>(conv_size_out);
            return retval;
        }

        /*!
         * Interpret the lump as raw bytes, C Compatibility mode
         * @note Caller must free
         * @return The entire contents of the lump as a new char array
         */
        char* read_bytes_ccompat()
        { return m_context->read_bytes_ccompat(nullptr); }

        /*!
         * Interpret the lump as an image
         * @return An optional image object
         */
        Optional<Image> read_image();

        /*!
         * Interpret the lump as a palette.
         * @return An optional palette object
         */
        Optional<Palette> read_palette();

        /*!
         * Opens an earlier version of the same lump. That is, if both "a.wad"
         * and "b.wad", loaded in that order, both contain the lump "LUMP", then
         * `wad::open` will use the version from "a.wad". Using this function
         * will set this instance to point to the lump in "b.wad" instead.
         *
         * @return true if an earlier version exists and was loaded, false otherwise
         */
        bool previous();

        /*!
         * Same as `has_previous`, but only checks that a previous lump exists.
         *
         * @return true if an earlier version exists, false otherwise
         */
        bool has_previous() const;

        /*!
         * Opens a newer version of the same lump.
         * @return true if a newer version exists and was loaded, false otherwise
         */
        bool next();

        /*!
         * Same as `has_next`, but only checks that a newer lump exists.
         * @return true if a newer version exists, false otherwise
         */
        bool has_next() const;
    };
  }
}

#endif //__LUMP__77664741
