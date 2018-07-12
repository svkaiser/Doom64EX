#ifndef __N64_ROM__65058706
#define __N64_ROM__65058706

#include <sstream>
#include <prelude.hh>

namespace imp {
  namespace sys {
    struct N64Loc {
        size_t offset;
        size_t size;
    };

    struct N64Version {
        StringView name;
        char country_id;
        char version_id;
        N64Loc iwad;
        N64Loc sn64;
        N64Loc sseq;
        N64Loc pcm;
    };

    class N64Rom {
        std::ifstream m_file;
        std::string m_error;
        std::string m_version;
        bool m_swapped {};
        const N64Version* m_rom_version {};

        std::istringstream m_load(const N64Loc& loc);

    public:
        N64Rom() = default;
        N64Rom(const N64Rom&) = delete;
        N64Rom(N64Rom&&) = default;

        /**!
         * Opens path for reading as a ROM.
         * @param path System path to ROM
         */
        explicit N64Rom(StringView path)
        { open(path); }

        N64Rom& operator=(const N64Rom&) = delete;
        N64Rom& operator=(N64Rom&&) = default;

        bool open(StringView path);

        bool is_open() const
        { return m_file.is_open(); }

        /**!
         * @return Error message if an error occurred. Empty string otherwise.
         */
        const std::string& error() const
        { return m_error; }

        /**!
         * @return ROM version string
         */
        const std::string& version() const
        { return m_version; }

        std::istringstream iwad();
        std::istringstream sn64();
        std::istringstream sseq();
        std::istringstream pcm();
    };
  }
}

#endif //__N64_ROM__65058706
