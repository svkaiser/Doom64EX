#ifndef __COMPLETION__59277656
#define __COMPLETION__59277656

#include "store.hh"

namespace imp {
  namespace cvar {
    class Completion {
        using range_type = StoreRange<PrefixPred>;
        using iterator = typename range_type::iterator;

        struct Iter {
            range_type range;
            iterator iter;
        };

        Optional<Iter> m_iter {};

        Ref m_ref {};

    public:
        void complete(StringView prefix);

        void next();

        Ref& get();

        void reset();
    };
  }
}

#endif //__COMPLETION__59277656
