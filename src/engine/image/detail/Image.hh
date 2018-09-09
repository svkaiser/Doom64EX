#ifndef __IMP_IMAGE__94221350
#define __IMP_IMAGE__94221350

#include <utility/types.hh>
#include "Scanline.hh"

namespace imp {
  namespace wad {
    class Lump;
  }

  struct SpriteOffset {
      int x {};
      int y {};

      SpriteOffset() = default;
      SpriteOffset(const SpriteOffset&) = default;
      SpriteOffset(int x, int y): x(x), y(y) {}
      SpriteOffset& operator=(const SpriteOffset&) = default;
  };

  namespace detail {
    constexpr uint16 image_pitch(uint16 width, uint16 align, size_t pixel_size)
    {
        width *= pixel_size;
        return static_cast<uint16>(width + ((align - (width & (align - 1))) & (align - 1)));
    }

    class ImageAdditional {
        SpriteOffset sprite_offset_ {};

    public:
        ImageAdditional() = default;
        ImageAdditional(const ImageAdditional&) = default;
        ImageAdditional& operator=(const ImageAdditional&) = default;

        const SpriteOffset& sprite_offset() const
        { return sprite_offset_; }

        void sprite_offset(const SpriteOffset& x)
        { sprite_offset_ = x; }
    };
  }

  constexpr auto num_image_formats = 5;
  enum class ImageFormat {
      png,
      doom,
      n64gfx,
      n64texture,
      n64sprite
  };

  template <class, class> class BasicImage;

  class Image : public detail::ImageAdditional {
      const PixelInfo* info_ {};
      uint16 width_ {};
      uint16 height_ {};
      uint16 pitch_ {};
      UniquePtr<char[]> data_ {};
      Palette pal_ {};
      size_t align_ {};

      template <class T>
      Image& assign_move_(T&& other)
      {
          width_ = other.width_;
          height_ = other.height_;
          pitch_ = other.pitch_;
          data_ = std::move(other.data_);

          other.width_ = 0;
          other.height_ = 0;
          other.pitch_ = 0;

          return *this;
      }

      template <class T>
      Image& assign_copy_(const T& other)
      {
          width_ = other.width_;
          height_ = other.height_;
          pitch_ = other.pitch_;

          if (other.data_) {
              data_ = make_unique<char[]>(size());
              std::copy_n(other.data_ptr(), size(), data_ptr());
          } else {
              data_ = nullptr;
          }

          return *this;
      }

  public:
      Image() = default;
      Image(Image&&) = default;
      Image(const Image& other):
          ImageAdditional(other),
          info_(other.info_),
          pal_(other.pal_)
      { assign_copy_(other); }

      Image(PixelFormat format, uint16 width, uint16 height, uint16 align = 1):
          info_(&get_pixel_info(format)),
          width_(width),
          height_(height),
          pitch_(detail::image_pitch(width, align, info_->width)),
          align_(align),
          data_(make_unique<char[]>(size())) {}

      template <class PixT, class PalT>
      Image(BasicImage<PixT, PalT>&& other):
          ImageAdditional(other),
          info_(&get_pixel_info<PixT>()),
          pal_(std::move(other.pal_))
      { assign_move_(other); }

      template <class PixT, class PalT>
      Image(const BasicImage<PixT, PalT>& other):
          ImageAdditional(other),
          info_(&get_pixel_info<PixT>()),
          pal_(other.pal_)
      { assign_copy_(other); }

      template <class PixT, class PalT>
      Image(const BasicImageView<PixT, PalT>& other):
          info_(&get_pixel_info<PixT>()),
          pal_(other.pal_)
      { assign_copy_(other); }

      template <class PixT>
      Image(BasicImage<PixT, void>&& other):
          info_(&get_pixel_info<PixT>())
      { assign_move_(other); }

      template <class PixT>
      Image(const BasicImage<PixT, void>& other):
          info_(&get_pixel_info<PixT>())
      { assign_copy_(other); }

      template <class PixT>
      Image(const BasicImageView<PixT, void>& other):
          info_(&get_pixel_info<PixT>())
      { assign_copy_(other); }

      Image(std::istream& s)
      { load(s); }

      Image(std::istream& s, ImageFormat f)
      { load(s, f); }

      Image& operator=(Image&&) = default;
      Image& operator=(const Image& other)
      {
          detail::ImageAdditional::operator=(other);
          info_ = other.info_;
          pal_ = other.pal_;
          return assign_copy_(other);
      }

      template <class PixT, class PalT>
      Image& operator=(BasicImage<PixT, PalT>&& other)
      {
          info_ = &get_pixel_info<PixT>();
          pal_ = std::move(other.pal_);
          return assign_move_(other);
      }

      template <class PixT, class PalT>
      Image& operator=(const BasicImage<PixT, PalT>& other)
      {
          info_ = &get_pixel_info<PixT>();
          pal_ = other.pal_;
          return assign_copy_(other);
      }

      template <class PixT>
      Image& operator=(BasicImage<PixT, void>&& other)
      {
          info_ = &get_pixel_info<PixT>();
          pal_ = {};
          return assign_move_(other);
      }

      template <class PixT>
      Image& operator=(const BasicImage<PixT, void>& other)
      {
          info_ = &get_pixel_info<PixT>();
          pal_ = {};
          return assign_copy_(other);
      }

      void load(std::istream& s);

      void load(std::istream& s, ImageFormat);

      void save(std::ostream&, ImageFormat) const;

      char* data_ptr()
      { return data_.get(); }

      const char* data_ptr() const
      { return data_.get(); }

      uint16 width() const
      { return width_; }

      uint16 height() const
      { return height_; }

      uint16 pitch() const
      { return pitch_; }

      size_t size() const
      { return info_ ? pitch_ * height_: 0; }

      size_t align() const
      { return align_; }

      const PixelInfo& pixel_info() const
      { return *info_; }

      PixelFormat pixel_format() const
      { return info_ ? info_->format : PixelFormat::none; }

      bool is_indexed() const
      { return info_ && info_->index; }

      Palette& palette()
      { return pal_; }

      const Palette& palette() const
      { return pal_; }

      void set_palette(const Palette& pal)
      { pal_ = pal; }

      void convert(PixelFormat format);

      void scale(size_t new_width, size_t new_height);

      void canvas(size_t new_width, size_t new_height);

      void flip_vertical();

      Scanline operator[](size_t i)
      { return { data_ptr() + pitch_ * i }; }

      ScanlineView operator[](size_t i) const
      { return { data_ptr() + pitch_ * i }; }

      template <class PixT, class PalT = void>
      BasicImageView<PixT, PalT> view_as() const
      { return *this; };

      template <class Func>
      auto match(Func func) const
      {
          auto cmatch = [this, &func](auto color)
          {
              using color_type = decltype(color);
              return func(this->view_as<color_type>());
          };

          auto imatch = [this, &func](auto index)
          {
              using index_type = decltype(index);
              auto cmatch = [this, &func](auto color)
              {
                  using color_type = decltype(color);
                  return func(this->view_as<index_type, color_type>());
              };

              return match_color(this->palette().pixel_format(), cmatch);
          };

          return match_pixel(pixel_format(), imatch, cmatch);
      }
  };

  template <class PixT, class PalT = void>
  class BasicImage : public detail::ImageAdditional {
      friend class Image;

      uint16 width_ {};
      uint16 height_ {};
      uint16 pitch_ {};
      UniquePtr<char[]> data_ {};
      BasicPalette<PalT> pal_ {};

      template <class T>
      BasicImage& assign_(const T& other)
      {
          width_ = other.width();
          height_ = other.height();
          pitch_ = other.pitch();
          if (other.data_ptr()) {
              data_ = make_unique<char[]>(size());
              std::copy_n(other.data_ptr(), size(), data_.get());
          } else {
              data_ = nullptr;
          }
          return *this;
      }

  public:
      using value_type = BasicScanline<PixT, PalT>;

      BasicImage() = default;

      BasicImage(BasicImage&&) = default;

      BasicImage(const BasicImage& other)
      { assign_(other); }

      BasicImage(uint16 width, uint16 height, uint16 align = 1):
          width_(width),
          height_(height),
          pitch_(detail::image_pitch(width, align, sizeof(PixT))),
          data_(make_unique<char[]>(size())) {}

      BasicImage& operator=(const BasicImage& other)
      { return assign_(other); }

      char* data_ptr()
      { return data_.get(); }

      const char* data_ptr() const
      { return data_.get(); }

      const BasicPalette<PalT>& palette() const
      { return pal_; }

      void set_palette(const BasicPalette<PalT>& pal)
      { pal_ = pal; }

      uint16 width() const
      { return width_; }

      uint16 height() const
      { return height_; }

      uint16 pitch() const
      { return pitch_; }

      size_t size() const
      { return pitch_ * height_; }

      BasicScanline<PixT, PalT> operator[](size_t i)
      { return { width_, data_.get() + pitch_ * i, pal_ }; }
  };

  template <class PixT>
  class BasicImage<PixT, void> : detail::ImageAdditional {
      friend class Image;

      uint16 width_ {};
      uint16 height_ {};
      uint16 pitch_ {};
      UniquePtr<char[]> data_ {};

      template <class T>
      void assign_(const T& other)
      {
          width_ = other.width();
          height_ = other.height();
          pitch_ = other.pitch();
          data_ = make_unique<char[]>(size());
          if (other.data_ptr()) {
              std::copy_n(other.data_ptr(), size(), data_.get());
          }
      }

  public:
      using value_type = BasicScanline<PixT, void>;

      BasicImage() = default;

      BasicImage(BasicImage&&) = default;

      BasicImage(const BasicImage& other):
          width_(other.width_),
          height_(other.height_),
          pitch_(other.pitch_),
          data_(make_unique<char[]>(size()))
      {
          if (other.data_) {
              std::copy_n(other.data_.get(), size(), data_.get());
          }
      }

      BasicImage(uint16 width, uint16 height, uint16 align = 1):
          width_(width),
          height_(height),
          pitch_(detail::image_pitch(width, align, sizeof(PixT))),
          data_(make_unique<char[]>(size())) {}

      const PixelInfo& pixel_info() const
      { return get_pixel_info(PixT::format); }

      char* data_ptr()
      { return data_.get(); }

      const char* data_ptr() const
      { return data_.get(); }

      bool palette() const
      { return false; }

      void set_palette(bool)
      {}

      uint16 width() const
      { return width_; }

      uint16 height() const
      { return height_; }

      uint16 pitch() const
      { return pitch_; }

      size_t size() const
      { return pitch_ * height_; }

      BasicScanline<PixT, void> operator[](size_t i)
      { return { width_, data_ptr() + pitch_ * i }; }
  };

  template <class PixT, class PalT = void>
  class BasicImageView {
      friend class Image;

      uint16 width_ {};
      uint16 height_ {};
      uint16 pitch_ {};
      const char* data_ {};
      BasicPaletteView<PalT> pal_ {};

      BasicImageView(const Image& other):
          width_(other.width()),
          height_(other.height()),
          pitch_(other.pitch()),
          data_(other.data_ptr()),
          pal_(other.palette().count(), other.palette().data_ptr()) {}

  public:
      using value_type = BasicScanline<PixT, PalT>;
      using pixel_type = PixT;
      using palette_type = PalT;

      BasicImageView() = default;

      BasicImageView(const BasicImageView& other) = default;

      BasicImageView(const BasicImage<PixT, PalT>& other):
          width_(other.width()),
          height_(other.height()),
          pitch_(other.pitch()),
          data_(other.data_ptr()),
          pal_(other.palette()) {}

      const char* data_ptr() const
      { return data_; }

      const BasicPaletteView<PalT>& palette() const
      { return pal_; }

      uint16 width() const
      { return width_; }

      uint16 height() const
      { return height_; }

      uint16 pitch() const
      { return pitch_; }

      size_t size() const
      { return pitch_ * height_; }

      BasicScanlineView<PixT, PalT> operator[](size_t i) const
      { return { width_, data_ + pitch_ * i, pal_ }; }
  };

  template <class PixT>
  class BasicImageView<PixT, void> {
      friend class Image;

      uint16 width_ {};
      uint16 height_ {};
      uint16 pitch_ {};
      const char* data_ {};

      BasicImageView(const Image& other):
          width_(other.width()),
          height_(other.height()),
          pitch_(other.pitch()),
          data_(other.data_ptr()) {}

  public:
      using value_type = BasicScanline<PixT, void>;
      using pixel_type = PixT;
      using palette_type = void;

      BasicImageView() = default;

      BasicImageView(const BasicImageView&) = default;

      BasicImageView(const BasicImage<PixT, void>& other):
          width_(other.width()),
          height_(other.height()),
          pitch_(other.pitch()),
          data_(other.data_ptr()) {}

      const char* data_ptr() const
      { return data_; }

      bool palette() const
      { return false; }

      uint16 width() const
      { return width_; }

      uint16 height() const
      { return height_; }

      uint16 pitch() const
      { return pitch_; }

      size_t size() const
      { return pitch_ * height_; }

      BasicScanlineView<PixT, void> operator[](size_t i) const
      { return { width_, data_ + pitch_ * i }; };
  };

  using RgbImage = BasicImage<Rgb>;
  using Rgb565Image = BasicImage<Rgb565>;
  using RgbaImage = BasicImage<Rgba>;
  using Rgba5551Image = BasicImage<Rgba5551>;
  using I8RgbImage = BasicImage<Index8, Rgb>;
  using I8Rgb565Image = BasicImage<Index8, Rgb565>;
  using I8RgbaImage = BasicImage<Index8, Rgba>;
  using I8Rgba5551Image = BasicImage<Index8, Rgba5551>;
}

#endif //__IMP_IMAGE__94221350
