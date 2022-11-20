#pragma once

#include <cassert>

#pragma warning(push)
#pragma warning(disable: 4738)
#include <filesystem>
#pragma warning(pop)

#include <qc-core/core.hpp>
#include <qc-core/span.hpp>
#include <qc-core/vector.hpp>

namespace qc::image
{
    template <typename P>
    class Image
    {
        public:

        static constexpr int components{sizeof(P)};

        Image() noexcept = default;
        explicit Image(const ivec2 & size);
        Image(const ivec2 & size, P * pixels);
        Image(int width, int height);
        Image(int width, int height, P * pixels);

        Image(const Image &) = delete;
        Image(Image && other) noexcept;

        Image & operator=(const Image &) = delete;
        Image & operator=(Image && other) noexcept;

        ~Image() noexcept;

        const ivec2 & size() const noexcept;

        int width() const noexcept;

        int height() const noexcept;

        P * pixels() noexcept;
        const P * pixels() const noexcept;

        P & at(const ivec2 & p) noexcept;
        const P & at(const ivec2 & p) const noexcept;
        P & at(int x, int y) noexcept;
        const P & at(int x, int y) const noexcept;

        bool contains(const ivec2 & p) const noexcept;

        void fill(const P & color) noexcept;
        void fill(const ispan2 & region, const P & color) noexcept;
        void fill(const ivec2 & pos, const ivec2 & size, const P & color) noexcept;

        void outline(const ispan2 & region, const P & color) noexcept;
        void outline(const ivec2 & pos, const ivec2 & size, const P & color) noexcept;

        void horizontalLine(const ivec2 & pos, int length, const P & color) noexcept;

        void verticalLine(const ivec2 & pos, int length, const P & color) noexcept;

        void diagonalLine(const ivec2 & pos, const int length, int thickness, bool slope, const P & color) noexcept;

        void checkerboard(int squareSize, const P & backColor, const P & foreColor) noexcept;

        void copy(const Image & src, const ivec2 & dstPos) noexcept;
        void copy(const Image & src, const ispan2 & srcRegion, const ivec2 & dstPos) noexcept;

        P * release() noexcept;

        private:

        ivec2 _size{};
        P * _pixels{};
    };

    using GrayImage = Image<u8>;
    using GrayAlphaImage = Image<ucvec2>;
    using RgbImage = Image<ucvec3>;
    using RgbaImage = Image<ucvec4>;

    template <typename P> Image<P> read(const std::filesystem::path & file, bool allowComponentPadding);

    GrayImage readGrayImage(const std::filesystem::path & file);
    GrayAlphaImage readGrayAlphaImage(const std::filesystem::path & file, bool allowComponentPadding);
    RgbImage readRgbImage(const std::filesystem::path & file, bool allowComponentPadding);
    RgbaImage readRgbaImage(const std::filesystem::path & file, bool allowComponentPadding);

    template <typename P> void write(const Image<P> & image, const std::filesystem::path & file);
}

// INLINE IMPLEMENTATION ///////////////////////////////////////////////////////////////////////////////////////////////

namespace qc::image
{
    template <typename P>
    inline Image<P>::Image(const ivec2 & size) :
        Image(size.x, size.y)
    {}

    template <typename P>
    inline Image<P>::Image(const ivec2 & size, P * const pixels) :
        Image(size.x, size.y, pixels)
    {}

    template <typename P>
    inline Image<P>::Image(const int width, const int height) :
        Image(width, height, static_cast<P *>(::operator new(width * height * sizeof(P))))
    {}

    template <typename P>
    inline Image<P>::Image(const int width, const int height, P * const pixels) :
        _size{width, height},
        _pixels{pixels}
    {}

    template <typename P>
    inline Image<P>::Image(Image && other) noexcept :
        _size{other._size},
        _pixels{other._pixels}
    {
        other._size = {};
        other._pixels = nullptr;
    }

    template <typename P>
    inline Image<P> & Image<P>::operator=(Image && other) noexcept
    {
        _size = other._size;
        _pixels = other._pixels;
        other._size = {};
        other._pixels = nullptr;
        return *this;
    }

    template <typename P>
    inline Image<P>::~Image() noexcept
    {
        ::operator delete(_pixels);
    }

    template <typename P>
    inline const ivec2 & Image<P>::size() const noexcept
    {
        return _size;
    }

    template <typename P>
    inline int Image<P>::width() const noexcept
    {
        return _size.x;
    }

    template <typename P>
    inline int Image<P>::height() const noexcept
    {
        return _size.y;
    }

    template <typename P>
    inline P * Image<P>::pixels() noexcept
    {
        return _pixels;
    }

    template <typename P>
    inline const P * Image<P>::pixels() const noexcept
    {
        return _pixels;
    }

    template <typename P>
    inline P & Image<P>::at(const ivec2 & p) noexcept
    {
        return const_cast<P &>(static_cast<const Image *>(this)->at(p));
    }

    template <typename P>
    inline const P & Image<P>::at(const ivec2 & p) const noexcept
    {
        return at(p.x, p.y);
    }

    template <typename P>
    inline P & Image<P>::at(const int x, const int y) noexcept
    {
        return const_cast<P &>(static_cast<const Image *>(this)->at(x, y));
    }

    template <typename P>
    inline const P & Image<P>::at(const int x, const int y) const noexcept
    {
        assert(x >= 0 && x < _size.x && y >= 0 && y < _size.y);

        return _pixels[(_size.y - 1 - y) * _size.x + x];
    }

    template <typename P>
    inline bool Image<P>::contains(const ivec2 & p) const noexcept
    {
        return p.x >= 0 && p.y >= 0 && p.x < _size.x && p.y < _size.y;
    }
}
