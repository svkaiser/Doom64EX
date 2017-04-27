#ifndef __IMP_IMAGE_DATA__39077755
#define __IMP_IMAGE_DATA__39077755

#include <imp/Prelude>
#include <imp/Pixel>
#include <algorithm>

namespace imp {
  namespace detail {
    struct Palette {};
    struct PaletteRef {};

    struct dummy_t {
        template <class T>
        operator T() const
        { return {}; }
    };

    constexpr size_t image_pitch(int16 width, int16 align, size_t pixel_size)
    {
        auto pixel_width = pixel_size * width;
        return pixel_width + (pixel_width & (align - 1));
    }

    constexpr size_t image_size(int16 width, int16 height, int16 align, size_t pixel_size)
    { return image_pitch(width, align, pixel_size) * height; }

    template <class T>
    T* to_ptr(T* x)
    { return x; }

    template <class T>
    T* to_ptr(const UniquePtr<T>& x)
    { return x.get(); }

    template <class T>
    T* to_ptr(const UniquePtr<T[]>& x)
    { return x.get(); }

    template <class PixT, class PalT, bool Owned>
    class ImageData;

    template <>
    class ImageData<void, void, true> {
        template <class, class, bool> friend class ImageData;

        template <class PixT, class PalT, bool Owned>
        void assign_(const ImageData<PixT, PalT, Owned>& other)
        {
            width_ = other.width_;
            height_ = other.height_;
            align_ = other.align_;
            pal_ = other.pal_;
            if (other.data_) {
                info_ = &other.pixel_info();
                data_ = make_unique<char[]>(size());
                std::copy_n(to_ptr(other.data_), size(), data_.get());
            }
        }

    protected:
        const PixelInfo* info_ {};
        int16 width_ {};
        int16 height_ {};
        int16 align_ {};
        UniquePtr<char[]> data_ {};
        Palette pal_ {};

    public:
        ImageData() {}

        ImageData(ImageData&&) = default;

        template <class PixT, class PalT, bool Owned>
        ImageData(const ImageData<PixT, PalT, Owned>& other)
        { assign_(other); }

        ImageData& operator=(ImageData&&) = default;

        template <class PixT, class PalT, bool Owned>
        ImageData& operator=(const ImageData<PixT, PalT, Owned>& other)
        { assign_(other); }

        const PixelInfo& pixel_info() const
        { return *info_; }

        size_t size() const
        { return image_size(width_, height_, align_, info_->width); }
    };

    template <>
    class ImageData<void, void, false> {
        template <class, class, bool> friend class ImageData;

        template <class PixT, class PalT, bool Owned>
        void assign_(ImageData<PixT, PalT, Owned>& other)
        {
            width_ = other.width_;
            height_ = other.height_;
            align_ = other.align_;
            pal_ = other.pal_;
            if (other.data_) {
                info_ = &other.pixel_info();
                data_ = to_ptr(other.data_);
            }
        }

    protected:
        const PixelInfo* info_ {};
        int16 width_ {};
        int16 height_ {};
        int16 align_ {};
        char* data_ {};
        PaletteRef pal_ {};

    public:
        ImageData() {}

        ImageData(ImageData&&) = default;

        template <class PixT, class PalT, bool Owned>
        ImageData(ImageData<PixT, PalT, Owned>& other)
        { assign_(other); }

        ImageData& operator=(ImageData&&) = default;

        template <class PixT, class PalT, bool Owned>
        ImageData& operator=(ImageData<PixT, PalT, Owned>& other)
        { assign_(other); }

        const PixelInfo& pixel_info() const
        { return *info_; }

        size_t size() const
        { return image_size(width_, height_, align_, info_->width); }
    };

    template <class PixT>
    class ImageData<PixT, void, true> {
        static_assert(is_color<PixT>, "PixT must be a color type");
        template <class, class, bool> friend class ImageData;

    protected:
        int16 width_ {};
        int16 height_ {};
        int16 align_ {};
        UniquePtr<char[]> data_ {};
        static constexpr dummy_t pal_ {};

    public:
        ImageData() {}

        ImageData(ImageData&&) = default;

        ImageData(int16 width, int16 height, int16 align = 1):
            width_(width),
            height_(height),
            align_(align),
            data_(make_unique<char[]>(size())) {}

        template <bool Owned>
        ImageData(const ImageData<PixT, void, Owned>& other):
            width_(other.width_),
            height_(other.height_),
            align_(other.align_)
        {
            if (other.data_) {
                data_ = make_unique<char[]>(size());
                std::copy_n(to_ptr(other.data_), size(), data_.get());
            }
        }

        const PixelInfo& pixel_info() const
        { return get_pixel_info(detail::pixel_info<PixT>::format); }

        size_t size() const
        { return image_size(width_, height_, align_, detail::pixel_info<PixT>::width); }
    };

    template <class PixT>
    class ImageData<PixT, void, false> {
        static_assert(is_color<PixT>, "PixT must be a color type");
        template <class, class, bool> friend class ImageData;

    protected:
        int16 width_ {};
        int16 height_ {};
        int16 align_ {};
        char* data_ {};
        static constexpr dummy_t pal_ {};

    public:
        ImageData() {}

        ImageData(ImageData&&) = default;

        template <bool Owned>
        ImageData(ImageData<PixT, void, Owned>& other):
            width_(other.width_),
            height_(other.height_),
            align_(other.align_),
            data_(to_ptr(other.data_)) {}

        const PixelInfo& pixel_info() const
        { return get_pixel_info(detail::pixel_info<PixT>::format); }

        size_t size() const
        { return image_size(width_, height_, align_, detail::pixel_info<PixT>::width); }
    };
  }
}

#endif //__IMP_IMAGE_DATA__39077755
