/*
 * Incorporated from Eternity engine's w_zip.cpp
 */

#include <fstream>
#include <sstream>
#include <zlib.h>

#include "wad/idevice.hh"
#include "wad/wad_loaders.hh"

using namespace imp::wad;

namespace {
  template<class T>
  void read_into(std::istream &s, T &x)
  {
      s.read(reinterpret_cast<char *>(&x), sizeof(T));
  }

  constexpr const char *_local_file_sig = "PK\x3\x4";
  constexpr const char *_central_dir_sig = "PK\x1\x2";
  constexpr const char *_end_of_dir_sig = "PK\x5\x6";

#pragma pack(push, 1)
  struct LocalFileHeader {
      uint16 extr_version;   // Version needed to extract
      uint16 gp_flags;       // General purpose flags
      uint16 method;         // Compression method
      uint16 file_time;      // DOS file modification time
      uint16 file_date;      // DOS file modification date
      uint32 crc32;          // Checksum
      uint32 compressed;     // Compressed file size
      uint32 uncompressed;   // Uncompressed file size
      uint16 name_length;    // Length of file name following struct
      uint16 extra_length;   // Length of "extra" field following name

      // Following structure:
      // const char *filename;
      // const char *extra;
  };

  struct CentralDirEntry {
      uint16 made_by_version; // Version "made by"
      uint16 extr_version;    // Version needed to extract
      uint16 gp_flags;        // General purpose flags
      uint16 method;          // Compression method
      uint16 file_time;       // DOS file modification time
      uint16 file_date;       // DOS file modification date
      uint32 crc32;           // Checksum
      uint32 compressed;      // Compressed file size
      uint32 uncompressed;    // Uncompressed file size
      uint16 name_length;     // Length of name following structure
      uint16 extra_length;    // Length of extra field following name
      uint16 comment_length;  // Length of comment following extra
      uint16 disk_start_num;  // Starting disk # for file
      uint16 int_attribs;     // Internal file attribute bitflags
      uint32 ext_attribs;     // External file attribute bitflags
      uint32 local_offset;    // Offset to LocalFileHeader

      // Following structure:
      // const char *filename;
      // const char *extra;
      // const char *comment;
  };

  struct EndOfCentralDir {
      uint16 disk_num;            // Disk number (NB: multi-partite zips are NOT supported)
      uint16 central_dir_disk_no; // Disk number containing the central directory
      uint16 num_entries_on_disk; // Number of entries on this disk
      uint16 num_entries_total;   // Total entries in the central directory
      uint32 central_dir_size;    // Central directory size in bytes
      uint32 central_dir_offset;  // Offset of central directory
      uint16 zip_comment_length;  // Length of following zip file comment

      // Following structure:
      // const char *comment;
  };
#pragma pack(pop)

  static_assert(sizeof(LocalFileHeader) == 26, "ZIP LocalFileHeader struct must have a size of 26 bytes");
  static_assert(sizeof(CentralDirEntry) == 42, "ZIP CentralDirEntry struct must have a size of 42 bytes");
  static_assert(sizeof(EndOfCentralDir) == 18, "ZIP EndOfCentralDir struct must have a size of 18 bytes");

  void _find_first_central_dir(std::istream &s)
  {
      constexpr int64 end_size = 4 + sizeof(EndOfCentralDir);
      constexpr int64 max_size = 65536 + end_size;
      s.seekg(0, std::ios::end);
      int64 file_size = s.tellg();
      int64 minimum = std::max(int64 {}, file_size - max_size);

      s.seekg(-end_size, std::ios::end);
      for (int64 pos = s.tellg(); pos >= minimum; --pos) {
          char signature[4];
          s.read(signature, 4);
          if (memcmp(signature, _end_of_dir_sig, 4) == 0) {
              EndOfCentralDir dir;
              read_into(s, dir);

              if (dir.zip_comment_length + pos + end_size != file_size)
                  continue;

              if (dir.disk_num != 0 || dir.num_entries_on_disk != dir.num_entries_total)
                  throw "Multi-partite ZIPs are not supported.";

              s.seekg(dir.central_dir_offset);
              return;
          }
      }

      throw "End of Central Dir not found in ZIP";
  }

  String _normalize(StringView filename)
  {
      String r;
      for (auto ch : filename) {
          if (!std::isalnum(ch))
              break;
          r.push_back(static_cast<char>(std::toupper(ch)));
      }
      return r;
  }

  struct ZipInfo {
      String name;
      String real_name;
      bool compressed;
      size_t filepos;
      size_t size;
      wad::Section section;
  };

  class ZipDevice;

  class ZipLump : public wad::ILump {
      ZipDevice& device_;
      ZipInfo info_;

  public:
      ZipLump(ZipDevice& device, ZipInfo info):
          device_(device),
          info_(info) {}

      String name() const override
      { return info_.name; }

      Section section() const override
      { return info_.section; }

      String real_name() const override
      { return info_.real_name; }

      IDevice& device() override;

      UniquePtr<std::istream> stream() override;
  };

  class ZipDevice : public IDevice {
      std::ifstream stream_;
      size_t central_dir_pos_ {};

  public:
      explicit ZipDevice(std::ifstream&& stream):
          stream_(std::move(stream)) {}

      Vector<ILumpPtr> read_all() override
      {
          _find_first_central_dir(stream_);
          central_dir_pos_ = static_cast<size_t>(stream_.tellg());
          Vector<ILumpPtr> lumps;

          while (!stream_.eof()) {
              char sig[4];
              stream_.read(sig, 4);

              if (memcmp(sig, _central_dir_sig, 4) == 0) {
                  CentralDirEntry entry;
                  read_into(stream_, entry);

                  String filename(entry.name_length, 0);
                  stream_.read(&filename[0], entry.name_length);

                  // We don't care about no comments.
                  stream_.seekg(entry.comment_length + entry.extra_length, std::ios::cur);

                  if (entry.method != 0 && entry.method != 8) {
                      println(stderr, "Unsupported compression method for {}", filename);
                      continue;
                  }

                  // Probably a directory. Ignore it.
                  if (entry.uncompressed == 0 && entry.compressed == 0)
                      continue;

                  wad::Section section {};
                  auto loc = filename.find_first_of('/', 1);
                  if (loc != String::npos) {
                      auto section_name = _normalize(filename.substr(0, loc));
                      if (section_name == "GRAPHICS") {
                          section = wad::Section::graphics;
                      } else if (section_name == "TEXTURES") {
                          section = wad::Section::textures;
                      } else if (section_name == "SOUNDS") {
                          section = wad::Section::sounds;
                      } else if (section_name == "SPRITES") {
                          section = wad::Section::sprites;
                      } else {
                          section = wad::Section::normal;
                      }
                      loc++;
                  } else {
                      section = wad::Section::normal;
                      loc = 0;
                  }
                  auto name = _normalize(filename.substr(loc)).substr(0, 8);

                  auto lump_info = ZipInfo { name, "", entry.method == 8, entry.local_offset, entry.uncompressed, section };
                  auto lump_ptr = std::make_unique<ZipLump>(*this, lump_info);
                  lumps.emplace_back(std::move(lump_ptr));
              } else {
                  break;
              }
          }

          return lumps;
      }

      std::istream& stream()
      { return stream_; }
  };
}

IDevice& ZipLump::device()
{ return device_; }

UniquePtr<std::istream> ZipLump::stream()
{
        auto& s = device_.stream();

        // Check file signature
        char sig[4];
        s.seekg(info_.filepos);
        s.read(sig, 4);
        if (memcmp(sig, _local_file_sig, 4) != 0)
            throw std::runtime_error("Not a LocalFileHeader");

        LocalFileHeader header {};
        read_into(s, header);

        s.seekg(header.name_length + header.extra_length, std::ios::cur);

        String bytes(header.uncompressed, 0);
        z_stream zs {};
        char buffer[32] {};

        inflateInit2(&zs, -MAX_WBITS);
        zs.next_out = reinterpret_cast<Bytef*>(&bytes[0]);
        zs.avail_out = header.uncompressed;

        int code {};
        do {
            if (zs.avail_in == 0) {
                s.read(buffer, sizeof(buffer));
                zs.next_in = reinterpret_cast<Bytef*>(buffer);
                zs.avail_in = sizeof(buffer);
            }
            code = inflate(&zs, Z_SYNC_FLUSH);
        } while(code == Z_OK && zs.avail_out);

        inflateEnd(&zs);

        if (code != Z_OK && code != Z_STREAM_END)
            throw std::runtime_error("invalid inflate stream");

        if (zs.avail_out != 0)
            throw std::runtime_error("truncated deflate stream");

        return std::make_unique<std::istringstream>(std::move(bytes));
}

IDevicePtr wad::zip_loader(StringView name)
{
    std::ifstream file(name.to_string(), std::ios::binary);
    file.exceptions(file.badbit | file.failbit);

    char signature[4];
    read_into(file, signature);

    // The ZIP file either starts with a LocalFileHeader if there are files in it,
    // or EndOfCentralDir if it's empty. Therefore we check both.
    if (memcmp(signature, _local_file_sig, 4) != 0 && memcmp(signature, _end_of_dir_sig, 4) != 0)
        return nullptr;

    return std::make_unique<ZipDevice>(std::move(file));
}
