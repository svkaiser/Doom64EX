#ifndef __FLAGS__93667126
#define __FLAGS__93667126

#include <bitset>
#include <initializer_list>

namespace imp {
  namespace cvar {
    enum class Flag {
        /**! Don't save to configuration file */
        noconfig,

        /**! Hide the variable from the user */
        hidden,

        /**! Server variable - If we are server, broadcast it to clients */
        server,

        /**! Client variable - If we are connected to a server, send it to it */
        client,

        /**! This variable might be removed and readded in the future, so
         *  remember the config string for it */
        dynamic,

        /**! Requires a restart to take effect. All this really does is tell the
             user about it. */
        restart,
    };

    class FlagSet {
        std::bitset<6> m_set {};

    public:
        FlagSet() = default;
        FlagSet(const FlagSet&) = default;

        FlagSet& operator=(const FlagSet&) = default;

        FlagSet& operator|=(Flag f)
        { set(f); return *this; }

        FlagSet(Flag flag)
        { set(flag); }

        FlagSet& set(Flag flag)
        {
            m_set.set(static_cast<int>(flag));
            return *this;
        }

        FlagSet& set(const FlagSet& flag)
        {
            m_set |= flag.m_set;
            return *this;
        }

        FlagSet& reset(Flag flag)
        {
            m_set.reset(static_cast<int>(flag));
            return *this;
        }

        FlagSet& reset(const FlagSet& flag)
        {
            m_set &= ~flag.m_set;
            return *this;
        }

        bool test(Flag flag) const
        {
            return m_set.test(static_cast<int>(flag));
        }

        bool is_subset_of(const FlagSet& flags) const
        { return (m_set & flags.m_set) == m_set; }
    };

    inline FlagSet operator|(FlagSet a, Flag b)
    { a.set(b); return a; }

    inline FlagSet operator|(Flag a, Flag b)
    { FlagSet f = a; f.set(b); return f; }

  } // cvar namespace
} // imp namespace

#endif //__FLAGS__93667126
