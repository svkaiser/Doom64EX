#ifndef DOOM64EX_LUMP_HASH_HH
#define DOOM64EX_LUMP_HASH_HH

namespace imp {
  namespace wad {
    class LumpHash {
        uint32 hash_{};

    public:
        LumpHash() = default;

        LumpHash(LumpHash &&) = default;

        LumpHash(const LumpHash &) = default;

        LumpHash(StringView str) :
            hash_(1315423911) {
            for (int i = 0; i < 8 && str[i]; ++i) {
                int c = str[i];
                hash_ ^= (hash_ << 5) + std::toupper(c) + (hash_ >> 2);
            }
            hash_ &= 0xffff;
        }

        LumpHash(uint32 hash) :
            hash_(hash) {}

        uint32 get() const { return hash_; }

        LumpHash &operator=(LumpHash &&) = default;

        LumpHash &operator=(const LumpHash &) = default;

        bool operator<(LumpHash other) const { return hash_ < other.hash_; }
    };
  }
}

#endif //DOOM64EX_LUMP_HASH_HH
