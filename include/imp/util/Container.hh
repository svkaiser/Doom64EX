#ifndef __IMP_CONTAINER__04456477
#define __IMP_CONTAINER__04456477

#include <algorithm>
#include <memory>

namespace imp {
  enum class CType {
      unique,
      view,
      ref,
  };

  template <class T, CType Type>
  class Container;

  template <class T>
  class Container<class T, CType::unique> {
      std::unique_ptr<T[]> data_ {};

  public:
      Container() {}

      Container(size_t size):
          data_(std::make_unique<T[]>(size)) {}

      template <CType Type>
      Container(const Container<T, Type>& other, size_t size):
          Container(size)
      {
          if (other.data_) {
              std::copy_n(other.get(), size, data_.get());
          }
      }

      Container(Container&& other) = default;

      Container& operator=(Container&&) = default;

      T* get()
      { return data_.get(); }

      const T* get() const
      { return data_.get(); }
  };

  template <class T>
  class Container<class T, CType::ref> {
      T* data_;

  public:
  };
}

#endif //__IMP_CONTAINER__04456477