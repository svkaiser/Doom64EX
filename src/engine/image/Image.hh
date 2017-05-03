#ifndef __IMP_IMAGE__33434191
#define __IMP_IMAGE__33434191

#include <imp/Image>

namespace imp {
  struct ImageFormatIO {
      virtual ~ImageFormatIO() {}

      virtual Optional<Image> load(std::istream&) const
      { throw std::logic_error { "Loading is not implemented for this image format" }; }

      virtual void save(std::ostream&, const Image&) const
      { throw std::logic_error { "Saving is not implemented for this image format" }; }
  };

  namespace init {
    UniquePtr<ImageFormatIO> image_png();
    UniquePtr<ImageFormatIO> image_doom();
  }
}

#endif //__IMP_IMAGE__33434191
