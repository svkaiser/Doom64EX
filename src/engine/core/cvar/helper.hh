#ifndef __HELPER__62792571
#define __HELPER__62792571

#include "store.hh"

namespace imp::cvar {
  class Register {
      FlagSet m_flags;

  public:
      Register(const FlagSet& flags = {}):
          m_flags(flags) {}

      Register& set_flag(Flag flag)
      {
          m_flags.set(flag);
          return *this;
      }

      Register& set_flags(const FlagSet& flags)
      {
          m_flags.set(flags);
          return *this;
      }

      Register& reset_flag(Flag flag)
      {
          m_flags.reset(flag);
          return *this;
      }

      Register& reset_flags(const FlagSet& flags)
      {
          m_flags.reset(flags);
          return *this;
      }

      template <class T>
      Register& operator()(VarBase<T>& var, StringView name, StringView desc)
      {
          g_store->add(var, name, desc, m_flags);
          return *this;
      }
  };
}

#endif //__HELPER__62792571
