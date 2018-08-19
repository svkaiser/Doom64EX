#ifndef __PREDICATES__40898284
#define __PREDICATES__40898284

#include <prelude.hh>
#include "data.hh"

namespace imp {
  namespace cvar {
    /**! Dummy predicate that returns true for any input */
    class DummyPred {
    public:
        bool operator()(const String&, std::weak_ptr<Data>&) const
        { return true; }
    };

    /**! Flag predicate that returns true if the given flag set is a subset of the
       cvar's */
    class FlagPred {
        FlagSet m_flags;

    public:
        FlagPred(const FlagSet& flags):
            m_flags(flags) {}

        FlagPred& operator=(const FlagPred&) = default;

        bool operator()(const String&, std::weak_ptr<Data>& data) const
        {
            auto p = data.lock();
            return p && m_flags.is_subset_of(p->flags());
        }
    };

    /**! Predicate that returns true if the cvar's name starts with the given string */
    class PrefixPred {
        String m_prefix;

    public:
        PrefixPred(StringView prefix):
            m_prefix(prefix) {}

        bool operator()(const String& name, std::weak_ptr<Data>&) const
        {
            return name.find(m_prefix) != String::npos;
        }
    };
  } // cvar namespace
} // imp namespace

#endif //__PREDICATES__40898284
