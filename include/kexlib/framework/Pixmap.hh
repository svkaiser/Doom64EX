#ifndef __DOOM64EX_KEXLIB_FRAMEWORK_PIXMAP_HH__
#define __DOOM64EX_KEXLIB_FRAMEWORK_PIXMAP_HH__

#include <KexDef.hh>
#include <framework/Pixel.hh>

template<PixelFormat _Format = PixelFormat::Unknown>
class ScanlineIterator;
template<PixelFormat _Format = PixelFormat::Unknown>
class PixelIterator;
template<PixelFormat _Format = PixelFormat::Unknown>
class PixelReference;
template<PixelFormat _Format = PixelFormat::Unknown>
class PixmapMapped;

class Pixmap
{
    PixelFormat m_fmt;

    byte_t* m_pixels;
    size_t m_width;
    size_t m_height;

public:
    using iterator_type = ScanlineIterator<PixelFormat::Unknown>;

public:
    /*!
     * \brief Initialise an empty Pixmap object
     */
    Pixmap();

    /*!
     * \brief Move constructor
     * \remark The moved object may not be used after this call
     */
    Pixmap(Pixmap&&);

    /*!
     * \brief Copy constructor
     * \remark This doesn't use CoW, so it actually does copy the entire buffer
     */
    Pixmap(Pixmap const&);

    ~Pixmap();

    /*!
     * \brief Empty the object and free the buffer
     */
    void Reset();

    /*!
     * \brief
     * \return this
     */
    Pixmap& operator=(Pixmap const&);

    /*!
     * \brief
     * \return this
     */
    Pixmap& operator=(Pixmap&&);

    /*!
     * \brief Get a pointer to the beginning of line yIndex
     * \param yIndex The index of the line to get
     * \return Pointer to the beginning of the scanline
     */
    void* Scanline(size_t yIndex);

    /*!
     * \brief Get a const pointer to the beginning of the line yIndex
     * \param yIndex The index of the line to get
     * \return Const pointer to the beginning of the scanline
     */
    void const* Scanline(size_t yIndex) const;

    /*!
     * \return the pixmap width
     */
    auto GetWidth() const
    {
        return m_width;
    }

    /*!
     * \return the pixmap height
     */
    auto GetHeight() const
    {
        return m_height;
    }

    template<PixelFormat _Format>
    auto Map()
    {
        return PixmapMapped<_Format>(*this);
    }

    /*!
     * \brief Copy raw image data into a new Pixmap container
     * \param pxData
     * \param width
     * \param height
     * \param format
     * \return a new Pixmap object
     */
    static Pixmap FromData(void const* pxData, size_t width, size_t height, PixelFormat format);
};

template<PixelFormat _Format >
class PixmapMapped
{
    Pixmap& m_pixmap;

public:
    using iterator_type = ScanlineIterator<_Format>;

    PixmapMapped(Pixmap& pixmap):
        m_pixmap(pixmap)
    {
    }

    iterator_type begin()
    {
        return iterator_type(m_pixmap, 0);
    }

    iterator_type end()
    {
        return iterator_type(m_pixmap, m_pixmap.GetHeight());
    }
};

/*!
 * \brief Ranged iterator for Pixmap Scanlines
 */
template<PixelFormat _Format >
class ScanlineIterator
{
    Pixmap& m_pixmap;
    size_t m_yIndex;

public:
    ScanlineIterator(Pixmap& pixmap, size_t yIndex = 0)
        :
        m_pixmap(pixmap),
        m_yIndex(yIndex)
    {
    }

    /*!
     * \brief Prefixed ++ operator overload
     * \return this
     */
    ScanlineIterator& operator++()
    {
        ++m_yIndex;

        return *this;
    }

    /*!
     * \brief Compares the line of the other iterator to ours
     * \remark Both iterators must point to the same underlying Pixmap
     * \param other
     * \return
     */
    bool operator<(ScanlineIterator const& other) const
    {
        // TODO: Add a proper assertion system
        // assert(m_pixmap == other.m_pixmap);
        return m_yIndex < other.m_yIndex;
    }

    /*!
     * \brief Compares the line of the other iterator to ours
     * \remark Both iterators must point to the same underlying Pixmap
     * \param other
     * \return
     */
    bool operator!=(ScanlineIterator const& other) const
    {
        // TODO: Add a proper assertion system
        // assert(m_pixmap == other.m_pixmap);
        return m_yIndex != other.m_yIndex;
    }

    /*!
     * \return Pointer to the start of the scanline
     */
    void* operator*()
    {
        return m_pixmap.Scanline(m_yIndex);
    }
};

template<PixelFormat _Format>
class PixelIterator
{
    Pixmap& m_pixmap;
    void* m_pixel;

public:
    PixelIterator(Pixmap& pixmap, void* pixel)
        :
        m_pixmap(pixmap),
        m_pixel(pixel)
    {
    }

    PixelIterator& operator++()
    {
        ++m_pixel;
        return *this;
    }

    bool operator!=(PixelIterator const& other)
    {
        return m_pixel != other.m_pixel;
    }

    PixelReference<_Format> operator*()
    {
        return PixelReference<_Format>(m_pixmap, m_pixel);
    }
};

template<PixelFormat _Format >
class PixelReference
{
    Pixmap& m_pixmap;
    void* m_pixel;

public:
    PixelReference(Pixmap& pixmap, void* pixel)
        :
        m_pixmap(pixmap),
        m_pixel(pixel)
    {
    }
};

/*!
 * \brief Helper for C++11 range-based for loops
 * \param pixmap
 * \return
 */
inline Pixmap::iterator_type begin(Pixmap& pixmap)
{
    return Pixmap::iterator_type(pixmap);
}

inline Pixmap::iterator_type end(Pixmap& pixmap)
{
    return Pixmap::iterator_type(pixmap, pixmap.GetHeight());
}

template<PixelFormat _Format>
inline auto begin(ScanlineIterator<_Format>& iter)
{
    return PixelIterator<_Format>(iter.Pixmap(),);
}

#endif //__DOOM64EX_KEXLIB_FRAMEWORK_PIXMAP_HH__
