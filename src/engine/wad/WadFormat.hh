#include <imp/Wad>

namespace imp {
  namespace wad {
    struct Reader {
        Section section {};
        std::size_t id {};
        StringView name {};

        virtual ~Reader() {}

        virtual bool poll() = 0;
    };

    struct Format {
        using loader = UniquePtr<Format> (*)(StringView);

        virtual ~Format() {}

        virtual UniquePtr<Reader> reader() = 0;

        virtual Optional<Lump> find(std::size_t id) = 0;

        virtual Optional<Lump> find(StringView name) = 0;
    };

    UniquePtr<Format> doom_loader(StringView);
  }
}
