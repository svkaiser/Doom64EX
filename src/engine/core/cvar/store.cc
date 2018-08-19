#include "ref.hh"
#include "store.hh"
#include "completion.hh"
#include <iostream>

using namespace imp;
namespace internal = imp::cvar::internal;

namespace {
  using Vars = imp::radix_tree <String, std::weak_ptr<cvar::Data>>;

  String s_normalize(StringView name)
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
    String sname = s_normalize(name);

    auto it = m_vars.find(sname);
    if (it != m_vars.end()) {
        throw cvar::var_already_exists(name.to_string());
    }

    data->set_name(name);
    data->set_desc(desc);
    data->set_valid(true);
    data->set_flags(flags);

    m_vars.insert({ sname, data });

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

    m_user_values.insert({s_normalize(name), value.to_string() });
}

//
// find_type
//

namespace imp {
  namespace cvar {
    template <>
    Optional<cvar::IntRef> cvar::Store::find_type<int64>(StringView name)
    {
        return s_find<int64>(m_vars, s_normalize(name));
    }
  }
}

//
// find
//

Optional<cvar::Ref> cvar::Store::find(StringView name)
{
    auto it = m_vars.find(s_normalize(name));
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
namespace imp {
  namespace cvar {
    namespace internal {
      template <>
      Optional<IntRef> find<int64>(StringView name)
      { return g_store->find_type<int64>(name.to_string()); }
    }
  }
}

//
// Completion::complete
//
void cvar::Completion::complete(StringView prefix)
{
    auto norm = s_normalize(prefix);
    auto iter = g_store->iter_prefix(norm);

    if (iter.begin() == iter.end()) {
        m_iter = nullopt;
        m_ref.reset(nullptr);
    } else {
        m_iter = Iter { iter, iter.begin() };
        m_ref.reset(*m_iter->iter);
    }
}

//
// Completion::next
//
void cvar::Completion::next()
{
    if (!m_iter)
        return;

    ++m_iter->iter;
    if (m_iter->iter == m_iter->range.end()) {
        m_iter->iter = m_iter->range.begin();
    }

    m_ref.reset(*m_iter->iter);
}

//
// Completion::get
//
cvar::Ref& cvar::Completion::get()
{
    return m_ref;
}

//
// Completion::reset
//
void cvar::Completion::reset()
{
    m_iter = nullopt;
    m_ref.reset(nullptr);
}
