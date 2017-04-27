#ifndef __IMP_IMAGE__94221350
#define __IMP_IMAGE__94221350

#include "ImageData.hh"

namespace imp {
  namespace detail {
    template <class PixT, class PalT, bool Owned>
    class ImageCommon : public ImageData<PixT, PalT, Owned> {
        using Data = ImageData<PixT, PalT, Owned>;

    public:
        using Data::ImageData;

        char* data()
        { return to_ptr(this->data_); }

        const char* data() const
        { return to_ptr(this->data_); }

        int16 width() const
        { return this->width_; }

        int16 height() const
        { return this->height_; }

        int16 align() const
        { return this->align_; }
    };
  }

  enum class ImageFormat {
      none,
      doom,
      png
  };

  class Image : public detail::ImageCommon<void, void, true> {
      using Data = detail::ImageCommon<void, void, true>;

  public:
      using Data::ImageCommon;

      void save(std::ostream&, ImageFormat = ImageFormat::none);
  };

  template <class PixT, class PalT = void>
  using BasicImage = detail::ImageCommon<PixT, PalT, true>;
  template <class PixT, class PalT = void>
  using BasicImageRef = detail::ImageCommon<PixT, PalT, false>;

  using RgbImage = BasicImage<Rgb>;
  using RgbImageRef = BasicImageRef<Rgb>;
  using RgbaImage = BasicImage<Rgba>;
  using RgbaImageRef = BasicImageRef<Rgba>;
}

#endif //__IMP_IMAGE__94221350
