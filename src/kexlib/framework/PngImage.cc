#include <framework/Image>
#include <fmt/format.h>
#include <png.h>

using namespace kex::gfx;

namespace // anonymous
{

const uint8_t magic[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};

void _read_fn(png_structp structp, png_bytep *area, size_t size)
{
    std::istream &s = *(std::istream *) png_get_io_ptr(structp);

    s.read((char *) area, size);
}

class PngImageData : ImageData
{
    Pixmap_sp mPixmap;

public:
    PngImageData(std::istream &stream);

    virtual ~PngImageData();

    virtual Pixmap_sp data() const;
};

struct PngImageType : public ImageType
{
    virtual bool detect(std::istream &stream);

    virtual std::shared_ptr<ImageData> construct(std::istream &stream);
};

bool PngImageType::detect(std::istream &stream)
{
    uint8_t buf[sizeof magic];
    stream.read((char *) buf, sizeof buf);
    return std::equal(magic, magic + sizeof magic, buf);
}

std::shared_ptr<ImageData> PngImageType::construct(std::istream &stream)
{
    return std::shared_ptr<ImageData>((ImageData *) new PngImageData(stream));
}

PngImageData::PngImageData(std::istream &stream)
{
    png_structp structp = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (!structp)
        throw std::exception();

    png_infop infop = png_create_info_struct(structp);
    if (!infop)
    {
        png_destroy_read_struct(&structp, nullptr, nullptr);
        throw std::exception();
    }

    if (setjmp(png_jmpbuf(structp)))
    {
        png_destroy_read_struct(&structp, &infop, nullptr);
        throw std::exception();
    }

    png_set_read_fn(structp, &stream, (png_rw_ptr) _read_fn);

    // TODO: Read user chunk

    png_read_info(structp, infop);

    png_uint_32 width;
    png_uint_32 height;
    int bitDepth;
    int colorType;
    int interlaceType;
    png_get_IHDR(structp, infop, &width, &height, &bitDepth, &colorType, &interlaceType, nullptr, nullptr);

    fmt::println("width: {}, height: {}", width, height);

    // TODO: Do something with transparency

    // TODO: Do palette

    if (bitDepth == 16)
        png_set_strip_16(structp);

    // refresh data in IHDR chunk
    png_read_update_info(structp, infop);
    png_get_IHDR(structp, infop, &width, &height, &bitDepth, &colorType, &interlaceType, nullptr, nullptr);

    mPixmap = std::make_shared<Pixmap>(width, height, PixelFormat::rgb);
    uint8_t **rowPointers = new uint8_t*[height];

    for (size_t i = 0; i < height; i++)
        rowPointers[i] = mPixmap->scanline(i);

    png_read_image(structp, rowPointers);
    png_read_end(structp, infop);

    delete[] rowPointers;
}

PngImageData::~PngImageData()
{

}

Pixmap_sp PngImageData::data() const
{
    return mPixmap;
}

} // anonymous namespace

void init_png()
{
    static PngImageType type;
}