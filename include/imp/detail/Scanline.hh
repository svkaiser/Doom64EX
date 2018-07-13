#ifndef __IMP_SCANLINE__77828553
#define __IMP_SCANLINE__77828553

#include "Palette.hh"

namespace imp {
  template <class, class> class BasicImage;
  template <class, class> class BasicImageView;
  class Image;

  template <class PixT, class PalT>
  class BasicScanline {
      friend class BasicImage<PixT, PalT>;

      uint16 width_ {};
      char *data_ {};
      const BasicPalette<PalT>* pal_ {};

      BasicScanline(uint16 width, char *data, const BasicPalette<PalT>& pal):
          width_(width),
          data_(data),
          pal_(&pal){}

  public:
      BasicScanline() = default;

      using value_type = PalT;

      uint16 width() const
      { return width_; }

      char* data_ptr()
      { return data_; }

      const char* data_ptr() const
      { return data_; }

      PixT* data()
      { return reinterpret_cast<PixT*>(data_); }

      const PixT* data() const
      { return reinterpret_cast<const PixT*>(data_); }

      PixT& get(size_t i)
      { return data()[i]; }

      const PixT& get(size_t i) const
      { return data()[i]; }

      const PixT& index(size_t i) const
      { return data()[i]; }

      void index(size_t i, PixT x)
      { data()[i] = x; }

      PixT& operator[](size_t i)
      { return data()[i]; }

      const PixT& operator[](size_t i) const
      { return data()[i]; }
  };

  template <class PixT>
  class BasicScanline<PixT, void> {
      friend class BasicImage<PixT, void>;

      uint16 width_ {};
      char *data_ {};

      BasicScanline(uint16 width, char *data):
          width_(width),
          data_(data) {}

  public:
      BasicScanline() = default;

      uint16 width() const
      { return width_; }

      char* data_ptr()
      { return data_; }

      const char* data_ptr() const
      { return data_; }

      PixT* data()
      { return reinterpret_cast<PixT*>(data_); }

      const PixT* data() const
      { return reinterpret_cast<const PixT*>(data_); }

      PixT& get(size_t i)
      { return data()[i]; }

      const PixT& get(size_t i) const
      { return data()[i]; }

      PixT& operator[](size_t i)
      { return data()[i]; }

      const PixT& operator[](size_t i) const
      { return data()[i]; }
  };

  template <class PixT, class PalT>
  class BasicScanlineView {
      friend class BasicImage<PixT, PalT>;
      friend class BasicImageView<PixT, PalT>;

      uint16 width_ {};
      const char *data_ {};
      BasicPaletteView<PalT> pal_ {};

      BasicScanlineView(uint16 width, const char *data, BasicPaletteView<PalT> pal):
          width_(width),
          data_(data),
          pal_(pal) {}

  public:
      BasicScanlineView() = default;

      uint16 width() const
      { return width_; }

      const char* data_ptr() const
      { return data_; }

      const PixT* data() const
      { return reinterpret_cast<const PixT*>(data_); }

      const PixT& index(size_t i) const
      { return data()[i]; }

      const PixT& get(size_t i) const
      { return data()[i]; }

      const PalT& operator[](size_t i) const
      { return pal_[index(i).index]; }
  };

  template <class PixT>
  class BasicScanlineView<PixT, void> {
      friend class BasicImage<PixT, void>;
      friend class BasicImageView<PixT, void>;

      uint16 width_ {};
      const char *data_ {};

      BasicScanlineView(uint16 width, const char *data):
          width_(width),
          data_(data) {}

  public:
      BasicScanlineView() = default;

      uint16 width() const
      { return width_; }

      const char* data_ptr() const
      { return data_; }

      const PixT* data() const
      { return reinterpret_cast<const PixT*>(data_); }

      const PixT& get(size_t i) const
      { return data()[i]; }

      const PixT& operator[](size_t i) const
      { return data()[i]; }
  };

  class Scanline {
      friend class Image;
      char *data_ {};

      Scanline(char *data):
          data_(data) {}
  public:
      Scanline() {}

      Scanline(const Scanline&) = default;

      Scanline& operator=(const Scanline&) = default;

      char *data_ptr()
      { return data_; }

      const char *data_ptr() const
      { return data_; }
  };

  class ScanlineView {
      friend class Image;
      const char *data_ {};

      ScanlineView(const char *data):
          data_(data) {}
  public:
      ScanlineView() {}

      ScanlineView(const ScanlineView&) = default;

      ScanlineView& operator=(const ScanlineView&) = default;

      const char *data_ptr() const
      { return data_; }
  };
}

#endif //__IMP_SCANLINE__77828553
