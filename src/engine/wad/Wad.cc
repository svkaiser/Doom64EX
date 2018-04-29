#include <map>
#include <imp/App>
#include <imp/Image>
#include <algorithm>
#include <cassert>
#include "WadFormat.hh"

namespace {
  wad::Format::loader loaders_[] {
      wad::doom_loader,
      wad::rom_loader,
      wad::zip_loader
  };

  Vector<UniquePtr<wad::Format>> mounts_;

  Vector<wad::LumpInfo> lumps_;

  Array<Vector<wad::LumpInfo*>, wad::num_sections> sections_;

  app::StringParam iwad_path_("iwad");
}

StringView wad::BasicLump::lump_name() const
{ return lumps_[id_].lump_name; }

std::size_t wad::BasicLump::lump_index() const
{ return lumps_[id_].lump_index; }

wad::Section wad::BasicLump::section() const
{ return lumps_[id_].section; }

size_t wad::BasicLump::section_index() const
{ return lumps_[id_].section_index; }

String wad::BasicLump::as_bytes()
{
    auto& s = stream();
    s.seekg(0);
    return { std::istreambuf_iterator<char>(s), std::istreambuf_iterator<char>() };
}

gfx::Image wad::BasicLump::as_image()
{
    auto& s = stream();
    s.seekg(0);
    return { s };
}

gfx::Image wad::Lump::as_image()
{ return data_->as_image(); }

void wad::init()
{
    if (iwad_path_ && !wad::mount(iwad_path_.get())) {
        fatal("Could not mount IWAD at {}", iwad_path_.get());
    } else {
        auto path = app::find_data_file("doom64.wad");
        if (!path)
            fatal("Could not find IWAD");
        if (!wad::mount(*path))
            fatal("Could not mount IWAD at {}", *path);
    }

    if(auto path = app::find_data_file("doom64ex.pk3")) {
        if (!wad::mount(*path)) {
            fatal("Could not mount doom64ex.pk3");
        }
    } else {
        fatal("Could not find doom64ex.pk3");
    }

    wad::merge();
}

bool wad::mount(StringView path)
{
    wad::Format *format {};
    for (auto l : loaders_) {
        if (auto f = l(path)) {
            format = f.get();
            mounts_.emplace_back(std::move(f));
            break;
        }
    }

    if (!format)
        return false;

    auto mount = mounts_.size() - 1;
    auto new_lumps = format->read_all();
    for (auto& l : new_lumps)
        l.mount = mount;
    lumps_.insert(lumps_.begin(), std::make_move_iterator(new_lumps.begin()), std::make_move_iterator(new_lumps.end()));

    return true;
}

void wad::merge()
{
    // Use a stable sort to allow different versions of lumps.
    std::stable_sort(lumps_.begin(), lumps_.end(),
                     [](const LumpInfo &a, const LumpInfo &b) { return a.lump_name < b.lump_name; });

    // Clear the sections
    sections_.fill({});

    LumpInfo *prev_lump {};
    size_t index {};
    for (auto& l : lumps_) {
        if (!prev_lump || prev_lump->lump_name != l.lump_name) {
            auto &s = sections_[static_cast<size_t>(l.section)];
            l.section_index = s.size();
            s.emplace_back(&l);
            l.lump_index = index++;
            prev_lump = &l;
        } else {
            assert(l.section == prev_lump->section);
            l.lump_index = prev_lump->lump_index;
            l.section_index = prev_lump->section_index;
        }
    }
}

bool wad::have_lump(StringView name)
{
    return wad::find(name).have_value();
}

Optional<wad::Lump> wad::find(StringView name)
{
    auto it = std::lower_bound(lumps_.begin(), lumps_.end(), name,
                               [](const LumpInfo& a, const StringView& b) {
                                   return a.lump_name < b;
                               });

    if (it == lumps_.end() || it->lump_name != name)
        return nullopt;

    const auto& mount = mounts_[it->mount];

    if (auto l = mount->find(std::distance(lumps_.begin(), it), it->mount_index)) {
        assert(it->lump_name == l->lump_name());
        return { inplace, std::move(l) };
    }

    return nullopt;
}

Optional<wad::Lump> wad::find(size_t lump_id)
{
    auto it = std::lower_bound(lumps_.begin(), lumps_.end(), lump_id,
                               [](const LumpInfo& lhs, const size_t& rhs) {
                                   return lhs.lump_index < rhs;
                               });

    if (it == lumps_.end() || it->lump_index != lump_id)
        return nullopt;


    auto& lump = *it;
	assert(lump.mount < mounts_.size());
    const auto& mount = mounts_[lump.mount];

    size_t index = static_cast<size_t>(std::distance(lumps_.begin(), it));
    if (auto l = mount->find(index, lump.mount_index)) {
        assert(lump.lump_index == l->lump_index());
        return { inplace, std::move(l) };
    }

    return nullopt;
}

Optional<wad::Lump> wad::find(wad::Section section, size_t index)
{
    return wad::find(sections_[static_cast<size_t>(section)][index]->lump_index);
}

wad::LumpIterator wad::section(wad::Section section)
{
    return section;
}

size_t wad::section_size(wad::Section section)
{
    auto& s = sections_[static_cast<size_t>(section)];
    return s.empty() ? 0 : s.back()->section_index + 1;
}

wad::LumpIterator::LumpIterator(Section section):
    section_(section),
    lump_(std::move(*wad::find(section_, 0)))
{
}

wad::Lump& wad::LumpIterator::operator*()
{
    if (lump_.section_index() != index_) {
        lump_ = std::move(*wad::find(section_, index_));
    }
    return lump_;
}

bool wad::LumpIterator::has_next() const
{
    return (index_) < wad::section_size(section_);
}

void wad::LumpIterator::next()
{
    ++index_;
}
