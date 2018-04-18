#include <map>
#include <algorithm>
#include <iostream>
#include <boost/algorithm/string.hpp>

#include "cvar.hh"

namespace {
  struct CName {
      StringView name;
      String norm;

      CName(StringView name):
          name(name)
      {
          norm.resize(name.size());
          std::transform(name.begin(), name.end(), norm.begin(), ::toupper);
      }

      bool operator==(const CName& other) const
      { return norm == other.norm; }

      bool operator<(const CName& other) const
      { return norm < other.norm; }
  };

  auto& _global()
  {
      static struct {
          // TODO: Write a radix tree for this
          std::map<CName, Cvar*> properties;
          std::vector<Cvar*> new_properties;
      } global {};
      return global;
  }
}

Cvar::Cvar(StringView name, StringView description, int flags):
    name_(name.to_string()),
    description_(description.to_string()),
    flags_(flags)
{
    if (_global().properties.count(name)) {
        log::warn("Cvar with the name {} already exists!", name);
    }

    _global().properties.emplace(name, this);
    _global().new_properties.emplace_back(this);
}

Cvar::~Cvar()
{
    _global().properties.erase({ name_ });
}

void Cvar::update()
{
    if (is_network()) {
        STUB("Update networked property");
    }
}

std::vector<Cvar *> Cvar::all()
{
    std::vector<Cvar *> v;
    v.reserve(_global().properties.size());
    for (auto& p : _global().properties) {
        assert(p.second != nullptr);
        v.emplace_back(p.second);
    }
    return v;
}

Cvar* Cvar::find(StringView name)
{
    auto it = _global().properties.find(name);
    return it != _global().properties.end() ? it->second : nullptr;
}

Vector<Cvar *> Cvar::partial(StringView prefix)
{
    // Search will be optimised when radix tree gets implemented.
    Vector<Cvar *> list;

    if (prefix.empty())
        return list;

    auto lower = boost::trim_copy(prefix.to_string());
    boost::to_lower(lower);
    bool found = false;
    for (auto it : _global().properties) {
        if (lower.length() > it.first.norm.size())
            continue;

        if (lower == it.first.norm.substr(lower.length())) {
            list.emplace_back(it.second);
            found = true;
        } else if (found) {
            // std::map has sorted keys. If we've found some but stopped finding more,
            // it means we've gone too far.
            break;
        }
    }

    return list;
}
