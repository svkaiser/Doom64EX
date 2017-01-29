#include <map>
#include "WadFormat.hh"

namespace {
  wad::Format::loader _loaders[] {
      wad::doom_loader
  };

  std::vector<UniquePtr<wad::Format>> _mounts;

  struct LumpId {
      String name {};
      std::size_t mount {};
      std::size_t id {};
      wad::Section section {};
      std::size_t index {};
      std::size_t section_index {};
  };

  std::map<String, LumpId> _lumps;

  std::vector<LumpId *> _lumps_by_id;

  std::map<wad::Section, std::pair<std::size_t, std::size_t>> _lumps_by_section;

  std::map<wad::Section, std::vector<LumpId *>> _group_by_section;
}

bool wad::mount(StringView path)
{
    wad::Format *format {};
    for (auto l : _loaders) {
        if (auto f = l(path)) {
            format = f.get();
            _mounts.emplace_back(std::move(f));
            break;
        }
    }

    if (!format)
        return false;

    auto mount = _mounts.size() - 1;
    auto reader = format->reader();
    while (reader->poll()) {
        auto it = _lumps.find(reader->name);
        if (it != _lumps.end()) {
            auto& name = it->first;
            auto& id = it->second;
            if (id.section != reader->section) {
                fatal("Unexpected WAD Lump section {} when loading \"{}:{}\"\n"
                      "Previously defined as {}",
                      path, name, to_string(reader->section), to_string(id.section));
            }
            id.mount = mount;
            id.id = reader->id;
        } else {
            auto pair = _lumps.emplace(reader->name, LumpId { reader->name, mount, reader->id, reader->section });
            _group_by_section[reader->section].emplace_back(&pair.first->second);
        }
    }
}

void wad::merge()
{
    std::size_t index = 0;
    _lumps_by_id.clear();
    auto section = [&](wad::Section section)
        {
            std::pair<std::size_t, std::size_t> pair;
            pair.first = _lumps_by_id.size();
            auto& v = _group_by_section[section];
            _lumps_by_id.insert(_lumps_by_id.end(), v.begin(), v.end());
            std::size_t section_index {};
            for (auto& x : v) {
                x->index = index++;
                x->section_index = section_index++;
            }
            v.clear();
            pair.second = _lumps_by_id.size();
            _lumps_by_section[section] = pair;
        };
    section(wad::Section::normal);
    section(wad::Section::textures);
    section(wad::Section::graphics);
    section(wad::Section::sprites);
    section(wad::Section::sounds);

    _group_by_section.clear();
}

bool wad::have_lump(StringView name)
{
    return _lumps.count(name);
}

Optional<wad::Lump> wad::find(StringView name)
{
    auto it = _lumps.find(name);
    if (it == _lumps.end())
        return nullopt;

    const auto& id = it->second;
    const auto& mount = _mounts[id.mount];

    /* Try to find lump by id (might be faster) */
    if (auto l = mount->find(id.id)) {
        l->index = it->second.index;
        l->section_index = it->second.section_index;
        return l;
    }

    /* Otherwise, try to find lump by name */
    return mount->find(name);
}

Optional<wad::Lump> wad::find(std::size_t index)
{
    if (index >= _lumps_by_id.size())
        return nullopt;

    const auto& id = *_lumps_by_id[index];
    const auto& mount = _mounts[id.mount];

    /* Try to find lump by id (might be faster) */
    if (auto l = mount->find(id.id)) {
        l->index = id.index;
        l->section_index = id.section_index;
        return l;
    }

    /* Otherwise, try to find lump by name */
    return nullopt;
}

wad::LumpIterator wad::section(wad::Section section)
{
    auto& p = _lumps_by_section[section];
    return { p.first, p.second };
}

wad::LumpIterator::LumpIterator(std::size_t begin, std::size_t end):
    index_(begin),
    begin_(begin),
    end_(end)
{
    if (index_ < end_) {
        lump_ = *wad::find(index_);
    }
}

wad::LumpIterator& wad::LumpIterator::operator++()
{
    if (++index_ < end_) {
        lump_ = *wad::find(index_);
    }

    return *this;
}

const wad::Lump& wad::LumpIterator::operator[](std::size_t i)
{
    return *wad::find(begin_ + i);
}

