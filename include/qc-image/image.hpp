#pragma once

#include <filesystem>

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
        explicit Image(const ivec2 & size, P * pixels);

        Image(const Image &) = delete;
        Image(Image && other) noexcept;

        Image & operator=(const Image &) = delete;
        Image & operator=(Image && other) noexcept;

        ~Image() noexcept;

        const ivec2 & size() const noexcept;

        P * pixels() noexcept;
        const P * pixels() const noexcept;

        P & at(const ivec2 & p) noexcept;
        const P & at(const ivec2 & p) const noexcept;
        P & at(int x, int y) noexcept;
        const P & at(int x, int y) const noexcept;

        void fill(const P & color) noexcept;
        void fill(const ispan2 & region, const P & color) noexcept;
        void fill(const ivec2 & pos, const ivec2 & size, const P & color) noexcept;

        void outline(const ispan2 & region, const P & color) noexcept;
        void outline(const ivec2 & pos, const ivec2 & size, const P & color) noexcept;

        void copy(const Image & src, const ivec2 & dstPos) noexcept;
        void copy(const Image & src, const ispan2 & srcRegion, const ivec2 & dstPos) noexcept;

        private:

        ivec2 _size{};
        P * _pixels{};
    };

    using GrayImage = Image<u8>;
    using GrayAlphaImage = Image<ucvec2>;
    using RgbImage = Image<ucvec3>;
    using RgbaImage = Image<ucvec4>;

    template <typename P> Image<P> read(const std::filesystem::path & file);

    template <typename P> void write(const Image<P> & image, const std::filesystem::path & file);
}

// INLINE IMPLEMENTATION ///////////////////////////////////////////////////////////////////////////////////////////////

namespace qc::image
{
    template <typename P>
    inline Image<P>::Image(const ivec2 & size) :
        Image(size, static_cast<P *>(::operator new(size.x * size.y * sizeof(P))))
    {}

    template <typename P>
    inline Image<P>::Image(const ivec2 & size, P * const pixels) :
        _size{size},
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
        return _pixels[(_size.y - 1 - y) * _size.x + x];
    }
}
