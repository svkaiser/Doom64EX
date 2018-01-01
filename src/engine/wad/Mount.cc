#include <map>
#include <imp/App>
#include <imp/Image>
#include <algorithm>
#include "Mount.hh"

namespace {
  wad::mount_loader *loaders_[] {
      wad::doom_loader,
          wad::rom_loader,
          wad::zip_loader
          };

  Vector<UniquePtr<wad::Mount>> mounts_;

  Vector<wad::LumpInfo> lumps_;

  Array<Vector<wad::LumpInfo*>, wad::num_sections> sections_;

  app::StringParam iwad_path_("iwad");
}

void wad::init()
{
    if (iwad_path_ && !wad::mount(iwad_path_.get())) {
        fatal("Could not mount IWAD at {}", iwad_path_.get());
    } else {
        auto path = app::find_data_file("doom64.rom");
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
    wad::Mount *format {};
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
        l.mount_index = mount;
    lumps_.insert(lumps_.begin(), std::make_move_iterator(new_lumps.begin()), std::make_move_iterator(new_lumps.end()));

    return true;
}

void wad::merge()
{
    // Use a stable sort to allow different versions of lumps.
    std::stable_sort(lumps_.begin(), lumps_.end(),
                     [](const LumpInfo &a, const LumpInfo &b) { return a.name < b.name; });

    // Clear the sections
    sections_.fill({});

    LumpInfo *prev_lump {};
    size_t index {};
    for (auto& l : lumps_) {
        if (!prev_lump || prev_lump->name != l.name) {
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
    return bool(wad::find(name));
}

Optional<wad::Lump> wad::find(StringView name)
{
    auto it = std::lower_bound(lumps_.begin(), lumps_.end(), name,
                               [](const LumpInfo& a, const StringView& b) {
                                   return a.name < b;
                               });

    if (it == lumps_.end() || it->name != name)
        return nullopt;

    return wad::find(it->lump_index);
}

Optional<wad::Lump> wad::find(size_t lump_id)
{
    auto it = std::lower_bound(lumps_.begin(), lumps_.end(), lump_id,
                               [](const LumpInfo& lhs, const size_t& rhs) {
                                   return lhs.lump_index < rhs;
                               });

    if (it == lumps_.end() || it->lump_index != lump_id)
        return nullopt;


    return wad::Lump { *it };
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
    section_(section) {}

wad::Lump wad::LumpIterator::operator*()
{
    return wad::find(section_, index_).value();
}

bool wad::LumpIterator::has_next() const
{
    return (index_) < wad::section_size(section_);
}

void wad::LumpIterator::next()
{
    ++index_;
}

wad::Lump::Lump(StringView name):
    Lump(std::move(wad::find(name).value()))
{
}

wad::LumpBuffer* wad::Lump::buffer()
{
    if (buffer_)
        return buffer_.get();

    const auto& mount = mounts_[info_->mount_index];

    mount->set_buffer(*this, info_->index);
    return buffer_.get();
}

StringView wad::Lump::lump_name() const
{
    return info_->name;
}

std::size_t wad::Lump::lump_index() const
{
    return info_->lump_index;
}

wad::Section wad::Lump::section() const
{
    return info_->section;
}

std::size_t wad::Lump::section_index() const
{
    return info_->section_index;
}

String wad::Lump::as_bytes()
{
    auto& s = stream();
    s.seekg(0, s.end);
    auto size = static_cast<size_t>(s.tellg());
    s.seekg(0);

    String buf;
    buf.resize(size);

    s.read(&buf[0], size);

    return buf;
}

char *wad::Lump::bytes_ptr()
{
    auto& s = stream();
    s.seekg(0, s.end);
    auto size = static_cast<size_t>(s.tellg());
    s.seekg(0);

    auto buf = new char[size];
    s.read(buf, size);

    return buf;
}

wad::Mount &wad::Lump::source()
{
    return *mounts_[info_->mount_index];
}
