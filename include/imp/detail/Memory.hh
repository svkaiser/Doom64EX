#ifndef __IMP_MEMORY__86310478
#define __IMP_MEMORY__86310478

#include <memory>

namespace imp {
  /*! Container type */
  enum struct CType {
      owner, /*!< Container is an owner, and is responsible for deallocation of its members */
      ref,   /*!< Container is a mutable reference */
      view   /*!< Container is an immutable reference */
  };

  template <class T, class Deleter = std::default_delete<T>>
  using UniquePtr = std::unique_ptr<T, Deleter>;

  template <class T>
  using SharedPtr = std::shared_ptr<T>;

  template <class T>
  using WeakPtr = std::weak_ptr<T>;

  template <class T, class... Args>
  UniquePtr<T> make_unique(Args&&... args)
  { return std::make_unique<T>(std::forward<Args>(args)...); }

  template <class T, class... Args>
  SharedPtr<T> make_shared(Args&&... args)
  { return std::make_shared<T>(std::forward<Args>(args)...); }

  template <class T>
  SharedPtr<T> make_shared_array(size_t size)
  { return { new T[size], [](T *mem) { delete[] mem; } }; }

  template <class T>
  decltype(auto) move(T&& x)
  { return std::move(std::forward<T>(x)); }

  template <class T>
  T* get_ptr(T* x)
  { return x; }

  template <class T>
  T* get_ptr(UniquePtr<T>& x)
  { return x.get(); }

  template <class T>
  const T* get_ptr(const UniquePtr<T>& x)
  { return x.get(); }

  template <class T>
  T* get_ptr(UniquePtr<T[]>& x)
  { return x.get(); }

  template <class T>
  const T* get_ptr(const UniquePtr<T[]>& x)
  { return x.get(); }

  template <class T>
  const T* get_ptr(const SharedPtr<T>& x)
  { return x.get(); }

  using size_t = std::size_t;
}

#endif //__IMP_MEMORY__86310478
