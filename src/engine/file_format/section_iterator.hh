#ifndef __SECTION_ITERATOR__88251771
#define __SECTION_ITERATOR__88251771

#include "metadata.hh"

namespace imp {
  namespace wad {
    class SectionIterator {
    public:
        virtual ~SectionIterator() {}
        virtual const Metadata& operator*() const = 0;
        virtual SectionIterator& operator++() = 0;
    };

    class MetaSectionIterator {
        Vector<SectionIterator> iters_;

    protected:
        MetaSectionIterator(ArrayView<SectionIterator> iters):
            iters_(iters.to_vector()) {}
    };
  }
}

#endif //__SECTION_ITERATOR__88251771
