#include <map>
#include <platform/app.hh>
#include <imp/Image>
#include <algorithm>
#include "wad.hh"

using namespace imp::wad;

namespace {
  Vector<IDeviceLoader*> device_loaders_;

  bool dirty_ {};

  Vector<IDevicePtr> devices_;

  Array<Vector<ILumpPtr>, num_sections> section_lumps_;

  app::StringParam iwad_path_("iwad");
}

bool wad::add_device(IDevicePtr device)
{
    dirty_ = true;

    auto lumps = device->read_all();
    for (auto& lump : lumps) {
        auto& list = section_lumps_[static_cast<size_t>(lump->section())];
        list.emplace_back(std::move(lump));
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
    // Use a stable sort to allow multiple versions of the same lumps.
    size_t lump_index {};
    for (size_t i {}; i < num_sections; ++i) {
        auto& lumps = section_lumps_[i];
        std::stable_sort(lumps.begin(), lumps.end(), [](const ILumpPtr& a, const ILumpPtr& b) { return a->name() < b->name(); });

        size_t section_index {};
        ILump *prev_lump{};
        for (auto &l : lumps) {
            if (!prev_lump || prev_lump->name() != l->name()) {
                l->set_section_index(section_index++);
                l->set_lump_index(lump_index++);
                prev_lump = l.get();
            } else {
                l->set_section_index(prev_lump->section_index());
                l->set_lump_index(prev_lump->lump_index());
            }
        }
    }

    dirty_ = false;
}

Optional<Lump> wad::open(Section section, StringView name)
{
    auto& lumps = section_lumps_[static_cast<size_t>(section)];

    auto it = std::lower_bound(lumps.begin(), lumps.end(), name,
                               [](const ILumpPtr& a, const StringView& b) {
                                   return a->name() < b;
                               });

    if (it == lumps.end() || it->get()->name() != name)
        return nullopt;

    return make_optional<Lump>(*(it->get()));
}

Optional<Lump> wad::open(Section section, size_t index)
{
    auto& lumps = section_lumps_[static_cast<size_t>(section)];

    for (auto& lump : lumps) {
        if (lump->section_index() == index) {
            return make_optional<Lump>(*lump);
        }
    }

    return nullopt;
}

Optional<Lump> wad::open(size_t index)
{
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
    return section_lumps_[static_cast<size_t>(section)];
}
