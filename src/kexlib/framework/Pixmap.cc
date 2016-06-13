#include <cstring>
#include <iostream>
#include <cstdlib>
#include "framework/Pixmap.hh"

static PixelFormatType fmts[]{
    {0, false},
    {24, false},
    {32, false},
    {8, true}
};

static uint8_t* alloc_data(size_t width, size_t height, PixelFormat format)
{
    uint8_t* data;

    data = new uint8_t[width * height * fmts[(int) format].bits / 8];

    return data;
}

Pixmap::Pixmap()
    :
    m_fmt(PixelFormat::Unknown),
    m_pixels(nullptr),
    m_width(0),
    m_height(0)
{
    std::cout << m_pixels << std::endl;
}

Pixmap::Pixmap(Pixmap const& other)
    :
    m_fmt(other.m_fmt),
    m_width(other.m_width),
    m_height(other.m_height)
{
    m_pixels = alloc_data(m_width, m_height, m_fmt);
}

Pixmap::Pixmap(Pixmap&& other)
    :
    m_fmt(other.m_fmt),
    m_width(other.m_width),
    m_height(other.m_height),
    m_pixels(other.m_pixels)
{
    other.m_pixels = nullptr;
}

Pixmap::~Pixmap()
{
    if (m_pixels) {
        delete[] m_pixels;
    }
}

void Pixmap::Reset()
{
    if (!m_pixels)
        return;

    delete[] m_pixels;
    m_pixels = nullptr;
    m_width = 0;
    m_height = 0;
    m_fmt = PixelFormat::Unknown;
}

Pixmap& Pixmap::operator=(Pixmap const& other)
{
    Reset();

    m_fmt = other.m_fmt;
    m_width = other.m_width;
    m_height = other.m_height;
    m_pixels = alloc_data(m_width, m_height, m_fmt);

    return *this;
}

Pixmap& Pixmap::operator=(Pixmap&& other)
{
    Reset();

    m_fmt = other.m_fmt;
    m_width = other.m_width;
    m_height = other.m_height;
    m_pixels = other.m_pixels;

    other.m_pixels = nullptr;

    return *this;
}

void* Pixmap::Scanline(size_t yIndex)
{
    return nullptr;
}

void const* Pixmap::Scanline(size_t yIndex) const
{
    return nullptr;
}

Pixmap Pixmap::FromData(void const* pxData, size_t width, size_t height, PixelFormat format)
{
    Pixmap retval;

    retval.m_width = width;
    retval.m_height = height;
    retval.m_fmt = format;
    retval.m_pixels = alloc_data(width, height, format);

    memcpy(retval.m_pixels, pxData, width * height * (fmts[(int) format].bits / 8));

    return retval;
}
