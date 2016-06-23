
#include <framework/Image>
#include <cxxabi.h>
#include <stdlib.h>

using namespace kex::gfx;

namespace
{

struct free_deleter
{
    void operator()(void *ptr)
    {
        free(ptr);
    }
};

std::vector<ImageType*> image_types;

template <class _T>
std::ostream &operator<<(std::ostream &s, const std::vector<_T> &vec)
{
    size_t size = vec.size();
    s << "std::vector<" << std::unique_ptr<char, free_deleter>(abi::__cxa_demangle(typeid(_T).name(), 0, 0, 0)).get() << ">{";

    for (size_t i = 0; i < size; i++)
    {
        auto &e = vec[i];
        s << e << ((i == size - 1) ? "" : ", ");
    }

    s.put('}');

    return s;
}

}

ImageData::~ImageData()
{
}

ImageType::ImageType()
{
    Image::add_type(this);
}

Image::Image()
{
}

Image::Image(std::istream &stream)
{
    std::fpos<mbstate_t> pos = stream.tellg();

    for (auto &type : image_types)
    {
        if (type->detect(stream))
        {
            stream.seekg(pos);
            mImage = type->construct(stream);
        }
    }
}

Pixmap_sp Image::data() const
{
    return mImage->data();
}

void Image::add_type(ImageType *type)
{
    image_types.push_back(type);
}
