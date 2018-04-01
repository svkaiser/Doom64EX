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
        ILump* context_;
        UniquePtr<std::istream> stream_ {};

    public:
        /*!
         * Don't allow creating a lump with no context
         */
        Lump() = delete;

        Lump(Lump&&) = default;
        Lump(const Lump&) = default;

        explicit Lump(ILump& context):
            context_(&context) {}

        Lump& operator=(Lump&&) = default;
        Lump& operator=(const Lump&) = default;

        /*!
         * Get the lump name of the form /^[A-Z0-9_]{1,8}$/
         * @return Lump name
         */
        String name() const
        { return context_->name(); }

        /*!
         * @return Section type
         */
        Section section() const
        { return context_->section(); }

        /*!
         * Get the real file name of the lump.
         * @return File name
         */
        String real_name() const
        { return context_->real_name(); }

        /*!
         * @return Section-local index
         */
        size_t section_index() const
        { return context_->section_index(); }

        /*!
         * @return Global lump index
         */
        size_t lump_index() const
        { return context_->lump_index(); }

        /*!
         * Get the device object that owns this lump.
         * @return Reference to device
         */
        IDevice& device()
        { return context_->device(); }

        /*!
         * Get a stream for reading
         * @return stream reference
         */
        std::istream& stream()
        {
            if (stream_)
                return *stream_;

            stream_ = std::move(context_->stream());
            if (!stream_)
                throw stream_error("Lump::stream()");

            return *stream_;
        }

        /*!
         * Interpret the lump as raw bytes
         * @return
         */
        String read_bytes()
        { return context_->read_bytes(); }

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
            char* retval = context_->read_bytes_ccompat(&conv_size_out);
            size_out = static_cast<IntT>(conv_size_out);
            return retval;
        }

        /*!
         * Interpret the lump as raw bytes, C Compatibility mode
         * @note Caller must free
         * @return The entire contents of the lump as a new char array
         */
        char* read_bytes_ccompat()
        { return context_->read_bytes_ccompat(nullptr); }

        /*!
         * Interpret the lump as an image
         * @return An optional image object
         */
        Optional<Image> read_image();
        //{ return context_->read_image(); }

        /*!
         * Interpret the lump as a palette.
         * @return An optional palette object
         */
        Optional<Palette> read_palette();
        //{ return context_->read_palette(); }
    };
  }
}

#endif //__LUMP__77664741
