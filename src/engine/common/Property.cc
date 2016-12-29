#include <map>
#include <algorithm>
#include <imp/Property>

namespace {
  auto& _global()
  {
      static struct {
          // TODO: Write a radix tree for this
          std::map<StringView, Property*> properties;
          std::vector<Property*> new_properties;
      } global {};
      return global;
  }
}

Property::Property(StringView name, StringView description, int flags):
    mName(name),
    mDescription(description),
    mFlags(flags)
{
    if (_global().properties.count(name)) {
        // TODO: Replace with an exception
        fatal("Property with the name {} already exists!", name);
    }

    _global().properties.emplace(name, this);
    _global().new_properties.emplace_back(this);
}

Property::~Property()
{
    _global().properties.erase(mName);
}

void Property::update()
{
    if (is_network()) {
        STUB("Update networked property");
    }
}

void Property::set_to_config()
{
    if (is_config()) {
//        auto value = Config::find_property(mName);
//        if (!value.empty()) {
//            set_value(value);
//            return;
//        }
    }
    set_to_default();
}

std::vector<Property *> Property::all()
{
    std::vector<Property *> v(_global().properties.size());
    for (auto& p : _global().properties)
        v.emplace_back(p.second);
    return v;
}

Property* Property::find(StringView name)
{
    auto it = _global().properties.find(name);
    return it != _global().properties.end() ? it->second : nullptr;
}

Vector<Property *> Property::partial(StringView prefix)
{
    // Search will be optimised when radix tree gets implemented.
    Vector<Property *> list;

    if (prefix.empty())
        return list;

    auto lower = prefix.trim().to_string();
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    bool found = false;
    for (auto it : _global().properties) {
        if (lower.length() > it.first.length())
            continue;

        if (lower == it.first) {
            list.push_back(it.second);
            found = true;
        } else if(found) {
            // std::map has sorted keys. If we've found some but stopped finding more,
            // it means we've gone too far.
            break;
        }
    }

    return list;
}