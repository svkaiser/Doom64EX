#ifndef __IMP_PALETTE__76334850
#define __IMP_PALETTE__76334850

#include <algorithm>
#include "utility/memory.hh"
#include "pixel.hh"

namespace imp {
  template <class, class> class BasicImageView;
  template <class> class BasicPalette;
  template <class> class BasicPaletteView;
  class Palette;

  template <class T>
  class BasicPalette {
      friend class Palette;

      size_t count_ {};
      SharedPtr<char> data_ {};

      BasicPalette& assign_(size_t count, const char* data)
      {
          count_ = count;

          if (data) {
              data_ = make_shared_array<char>(count_ * sizeof(T));
              std::copy_n(data, count_ * sizeof(T), data_.get());
          } else {
              data_ = nullptr;
          }

          return *this;
      }

      BasicPalette(size_t count, const char* data)
      { assign_(count, data); }

  public:
      using value_type = T;
      using size_type = std::size_t;
      using difference_type = std::ptrdiff_t;
      using reference = T&;
      using const_reference = const T&;
      using pointer = T*;
      using const_pointer = const T*;
      using iterator = pointer;
      using const_iterator = const_pointer;

      BasicPalette() = default;

      BasicPalette(BasicPalette &&) = default;

      BasicPalette(const BasicPalette& other) = default;

      BasicPalette(const BasicPaletteView<T>& other)
      { assign_(other.count(), other.data_ptr()); }

      BasicPalette(size_t count):
          count_(count),
          data_(make_shared_array<char>(count_ * sizeof(T))) {}

      BasicPalette& operator=(BasicPalette&&) = default;

      BasicPalette& operator=(const BasicPalette& other) = default;

      PixelFormat pixel_format() const
      { return T::format; }

      const PixelInfo& pixel_info() const
      { return get_pixel_info(T::format); }

      char* data_ptr()
      { return data_.get(); }

      const char* data_ptr() const
      { return data_.get(); }

      T* data()
      { return reinterpret_cast<T*>(data_.get()); }

      const T* data() const
      { return reinterpret_cast<const T*>(data_.get()); }

      size_t count() const
      { return count_; }

      reference at(size_type i)
      {
          if (i >= count_)
              throw std::out_of_range { "Palette out of range" };
          return data()[i];
      }

      const_reference at(size_type i) const
      {
          if (i >= count_)
              throw std::out_of_range { "Palette out of range" };
          return data()[i];
      }

      reference operator[](size_type i)
      { return data()[i]; }

      const_reference operator[](size_type i) const
      { return data()[i]; }

      iterator begin()
      { return data(); }

      const_iterator begin() const
      { return data(); }

      const_iterator cbegin() const
      { return data(); }

      iterator end()
      { return data() + count_; }

      const_iterator end() const
      { return data() + count_; }

      const_iterator cend() const
      { return data() + count_; }
  };

  template <class T>
  class BasicPaletteView {
      template <class, class> friend class BasicImageView;
      friend class Palette;

      size_t count_ {};
      const char* data_ {};

      BasicPaletteView(size_t count, const char* data):
          count_(count),
          data_(data) {}

  public:
      using value_type = T;
      using size_type = std::size_t;
      using difference_type = std::ptrdiff_t;
      using const_reference = const T&;
      using reference = const_reference ;
      using const_pointer = const T*;
      using pointer = const_pointer ;
      using iterator = const_pointer;
      using const_iterator = const_pointer;

      BasicPaletteView() = default;

      BasicPaletteView(const BasicPaletteView& other) = default;

      BasicPaletteView(const BasicPalette<T>& other):
          count_(other.count()),
          data_(other.data_ptr()) {}

      PixelFormat pixel_format() const
      { return detail::pixel_info<T>::format; }

      const PixelInfo& pixel_info() const
      { return get_pixel_info(pixel_format()); }

      const char* data_ptr() const
      { return data_; }

      const T* data() const
      { return reinterpret_cast<const T*>(data_); }

      size_t count() const
      { return count_; }

      const T& operator[](size_t i) const
      { return data()[i]; }

      const_iterator begin() const
      { return data(); }

      const_iterator cbegin() const
      { return data(); }

      const_iterator end() const
      { return data() + count_; }

      const_iterator cend() const
      { return data() + count_; }
  };

  class Palette {
      template <class> friend class BasicPalette;

      const PixelInfo* info_ {};
      size_t count_ {};
      SharedPtr<char> data_ {};

      template <class T>
      Palette& assign_(const T& other)
      {
          info_ = &other.pixel_info();
          count_ = other.count();
          if (count_) {
              auto size = count_ * info_->width;
              data_ = make_shared_array<char>(size);
              std::copy_n(other.data_ptr(), size, data_.get());
          } else {
              data_ = nullptr;
          }
          return *this;
      }

  public:
      Palette() = default;

      Palette(Palette&&) = default;

      Palette(const Palette& other)
      { assign_(other); }

      template <class T>
      Palette(BasicPalette<T>&& other):
          info_(&other.pixel_info()),
          count_(other.count()),
          data_(std::move(other.data_))
      {
          other.count_ = 0;
      }

      template <class T>
      Palette(const BasicPalette<T>& other)
      { assign_(other); }

      template <class T>
      Palette(const BasicPaletteView<T>& other)
      { assign_(other); }

      Palette(PixelFormat format, size_t count):
          info_(&get_pixel_info(format)),
          count_(count)
      {
          if (info_) {
              data_ = make_shared_array<char>(count_ * info_->width);
          }
      }

      Palette& operator=(Palette&&) = default;

      Palette& operator=(const Palette& other)
      { return assign_(other); }

      template <class T>
      Palette& operator=(BasicPalette<T>&& other)
      {
          info_ = &other.pixel_info();
          count_ = other.count();
          data_ = std::move(other.data_);
          other.count_ = 0;
          return *this;
      }

      template <class T>
      Palette& operator=(const BasicPalette<T>& other)
      { return assign_(other); }

      template <class T>
      BasicPalette<T> clone_as() const
      { return BasicPalette<T> { count_, data_.get() }; }

      char* data_ptr()
      { return data_.get(); }

      const char* data_ptr() const
      { return data_.get(); }

      const PixelInfo& pixel_info() const
      { return *info_; }

      PixelFormat pixel_format() const
      { return info_ ? info_->format : PixelFormat::none; }

      size_t count() const
      { return count_; }

      operator bool() const
      { return !!data_; }

      bool operator!() const
      { return !data_; }
  };

  using RgbPalette = BasicPalette<Rgb>;
  using RgbaPalette = BasicPalette<Rgba>;
  using Rgba5551Palette = BasicPalette<Rgba5551>;
}

#endif //__IMP_PALETTE__76334850
