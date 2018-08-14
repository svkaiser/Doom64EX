#include "ref.hh"
#include "store.hh"
#include <iostream>

using namespace imp;
namespace internal = imp::cvar::internal;

namespace {
  using Vars = std::unordered_map<String, std::weak_ptr<cvar::Data>>;

  String s_standard(StringView name)
  {
      String cname;

      for (auto c : name) {
          c = std::tolower(c);
          if ((c < 'a' || c > 'z') && (c < '0' || c > '9') && c != '_') {
              continue;
          }
          cname.push_back(c);
      }

      return cname;
  }

  template <class T>
  Optional<cvar::RefBase<T>> s_find(Vars& vars, const String& name)
  {
      auto it = vars.find(name);
      if (it == vars.end()) {
          return nullopt;
      }

      auto data = it->second.lock();
      if (!data) {
          vars.erase(it);
          return nullopt;
      }

      cvar::RefBase<T> ref { data };
      return ref;
  }
}

//
// global g_store
//
UniquePtr<cvar::Store> cvar::g_store = nullptr;

//
// private Store::p_add
//

void cvar::Store::p_add(std::shared_ptr<Data> data, StringView name, StringView desc, const FlagSet& flags)
{
    String sname = s_standard(name);
    log::warn("+ {}\n", sname);

    auto it = m_vars.find(sname);
    if (it != m_vars.end()) {
        throw cvar::var_already_exists(name.to_string());
    }

    data->set_name(name);
    data->set_desc(desc);
    data->set_valid(true);
    data->set_flags(flags);

    m_vars.emplace(sname, data);

    auto u_it = m_user_values.find(sname);
    if (u_it != m_user_values.end()) {
        data->set_value(u_it->second);

        if (!data->test_flag(cvar::Flag::dynamic)) {
            m_user_values.erase(u_it);
        }
    }
}

//
// Store::user_value
//

void cvar::Store::user_value(StringView name, StringView value)
{
    if (auto ref = find(name)) {
        *ref = value;

        if (!ref->is_dynamic())
            return;
    }

    m_user_values.emplace(s_standard(name), value.to_string());
}

//
// find_type
//

template <>
Optional<cvar::IntRef> cvar::Store::find_type<int64>(StringView name)
{
    return s_find<int64>(m_vars, s_standard(name));
}

//
// find
//

Optional<cvar::Ref> cvar::Store::find(StringView name)
{
    auto it = m_vars.find(s_standard(name));
    log::warn("? {}\n", s_standard(name));
    if (it == m_vars.end()) {
        return nullopt;
    }

    auto data = it->second.lock();
    if (!data) {
        m_vars.erase(it);
        return nullopt;
    }

    return cvar::Ref { data };
}

//
// internal find
//
template <>
Optional<cvar::IntRef> internal::find<int64>(StringView name)
{ return cvar::g_store->find_type<int64>(static_cast<String>(name)); }
