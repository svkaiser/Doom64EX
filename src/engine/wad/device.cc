#include <map>
#include <platform/app.hh>
#include <imp/Image>
#include <algorithm>
#include <unordered_map>
#include "wad.hh"

using namespace imp::wad;

namespace {
  Vector<IDeviceLoader*> device_loaders_;

  bool dirty_ {};

  Vector<IDevicePtr> devices_;

  class SectionLumps {
      std::vector<ILumpPtr> m_lumps;
      std::unordered_map<String, size_t> m_index_by_name;

  public:
      using iterator = typename std::vector<ILumpPtr>::iterator;

      void push_back(ILumpPtr&& lump)
      {
          auto it = m_index_by_name.find(lump->name());
          if (lump->name() == "?" || it == m_index_by_name.end()) {
              lump->set_section_index(m_lumps.size());
              m_index_by_name.emplace(lump->name(), m_lumps.size());
              m_lumps.push_back(std::move(lump));
          } else {
              std::swap(m_lumps[it->second], lump);
          }
      }

      std::pair<ILump*, size_t> find(StringView name)
      {
          auto it = m_index_by_name.find(name.to_string());
          if (it == m_index_by_name.end())
              return { nullptr, 0 };
          return { m_lumps[it->second].get(), it->second };
      }

      ILump* operator[](size_t index)
      {
          if (index >= m_lumps.size()) {
              return nullptr;
          }
          return m_lumps[index].get();
      }

      iterator begin()
      { return m_lumps.begin(); }

      iterator end()
      { return m_lumps.end(); }

      ArrayView<ILumpPtr> view()
      { return m_lumps; }
  };

  Array<SectionLumps, num_sections> section_lumps_;
}

bool wad::add_device(IDevicePtr device)
{
    auto lumps = device->read_all();
    for (auto& lump : lumps) {
        auto& list = section_lumps_[static_cast<size_t>(lump->section())];
        list.push_back(std::move(lump));
    }

    log::info("Added {} lumps from '{}'", lumps.size(), "");

    devices_.emplace_back(std::move(device));

    return true;
}

bool wad::add_device(StringView path)
{
    bool found {};
    for (auto l : device_loaders_) {
        if (auto d = l(path)) {
            found = add_device(std::move(d));
            break;
        }
    }

    if (!found) {
        log::error("Could not find an appropriate device loader for '{}'", path);
        return false;
    }

    return true;
}

bool wad::add_device_loader(IDeviceLoader& device_loader)
{
    device_loaders_.emplace_back(&device_loader);
    return false;
}

void wad::merge()
{
    size_t index {};
    for (auto& section : section_lumps_) {
        for (auto& lump : section) {
            lump->set_lump_index(index++);
        }
    }
    dirty_ = false;
}

Optional<Lump> wad::open(Section section, StringView name)
{
    auto& lumps = section_lumps_[static_cast<size_t>(section)];

    auto lump = lumps.find(name);
    if (!lump.first) {
        return nullopt;
    }

    return make_optional<Lump>(*lump.first);
}

Optional<Lump> wad::open(Section section, size_t index)
{
    auto& lumps = section_lumps_[static_cast<size_t>(section)];

    auto lump = lumps[index];
    if (!lump)
        return nullopt;
    return make_optional<Lump>(*lump);
}

Optional<Lump> wad::open(size_t index)
{
    assert(!dirty_);

    for (size_t i {}; i < num_sections; ++i) {
        auto &lumps = section_lumps_[i];

        for (auto &lump : lumps) {
            if (lump->lump_index() == index) {
                return make_optional<Lump>(*lump);
            }
        }
    }

    return nullopt;
}

ArrayView<ILumpPtr> wad::list_section(wad::Section section)
{
    return section_lumps_[static_cast<size_t>(section)].view();
}

bool Lump::previous()
{
//    const auto& sect = section_lumps_[static_cast<size_t>(section())];
//
//    if (m_offset >= sect.size())
//        return false;
//
//    auto& lump = sect[m_offset + 1];
//    if (lump_index() == lump->lump_index()) {
//        m_offset++;
//        m_context = lump.get();
//        return true;
//    }

    return false;
}

bool Lump::has_previous() const
{
//    const auto& sect = section_lumps_[static_cast<size_t>(section())];
//
//    if (m_offset >= sect.size())
//        return false;
//
//    const auto& lump = sect[m_offset + 1];
//    return lump_index() == lump->lump_index();
return false;
}

bool Lump::next()
{
//    const auto& sect = section_lumps_[static_cast<size_t>(section())];
//
//    if (m_offset == 0)
//        return false;
//
//    auto& lump = sect[m_offset - 1];
//    if (lump_index() == lump->lump_index()) {
//        m_offset--;
//        m_context = lump.get();
//        return true;
//    }

    return false;
}

bool Lump::has_next() const
{
//    const auto& sect = section_lumps_[static_cast<size_t>(section())];
//
//    if (m_offset == 0)
//        return false;
//
//    const auto& lump = sect[m_offset - 1];
//    return lump_index() == lump->lump_index();
return false;
}
