#pragma once

#include <cassert>

#include <filesystem>

#include <qc-core/core.hpp>
#include <qc-core/span.hpp>
#include <qc-core/vector.hpp>

namespace qc::image
{
    template <typename P> class ImageView;

    template <typename P>
    class Image
    {
        friend class ImageView<P>;
        friend class ImageView<const P>;

      public:

        static constexpr u32 components{sizeof(P)};

        Image() = default;
        explicit Image(uivec2 size);
        Image(uivec2 size, P * pixels);
        Image(u32 width, u32 height);
        Image(u32 width, u32 height, P * pixels);

        Image(const Image &) = delete;
        Image(Image && other);

        Image & operator=(const Image &) = delete;
        Image & operator=(Image && other);

        ~Image();

        void fill(const P & color);

        nodisc ImageView<P> view();
        nodisc ImageView<const P> view() const;
        nodisc ImageView<P> view(ivec2 position, uivec2 size);
        nodisc ImageView<const P> view(ivec2 position, uivec2 size) const;

        nodisc uivec2 size() const { return _size; }

        nodisc u32 width() const { return _size.x; };

        nodisc u32 height() const { return _size.y; };

        nodisc P * pixels() { return _pixels; };
        nodisc const P * pixels() const { return _pixels; };

        nodisc P * row(s32 y);
        nodisc const P * row(s32 y) const;

        nodisc P & at(ivec2 p);
        nodisc const P & at(ivec2 p) const;
        nodisc P & at(s32 x, s32 y);
        nodisc const P & at(s32 x, s32 y) const;

        P * release();

      private:

        uivec2 _size{};
        P * _pixels{};
    };

    using GrayImage = Image<u8>;
    using GrayAlphaImage = Image<ucvec2>;
    using RgbImage = Image<ucvec3>;
    using RgbaImage = Image<ucvec4>;

    template <typename P>
    class ImageView
    {
        using _ImageP = std::remove_const_t<P>;
        using _Image = std::conditional_t<std::is_const_v<P>, const Image<_ImageP>, Image<_ImageP>>;
        friend class Image<_ImageP>;
        friend class ImageView<std::remove_const_t<P>>;
        friend class ImageView<std::add_const_t<P>>;

      public:

        ImageView() = default;
        ImageView(_Image & image, ivec2 pos, uivec2 size);

        ImageView(const ImageView &) = default;
        ImageView(const ImageView<std::remove_const_t<P>> & other) requires std::is_const_v<P>;

        ImageView & operator=(const ImageView &) = default;
        ImageView & operator=(const ImageView<std::remove_const_t<P>> & other) requires std::is_const_v<P>;

        nodisc ImageView view(ivec2 position, uivec2 size) const;

        nodisc _Image * image() const { return _image; }

        nodisc ivec2 pos() const { return _pos; }

        nodisc uivec2 size() const { return _size; }

        nodisc u32 width() const { return _size.x; }

        nodisc u32 height() const { return _size.y; }

        nodisc P * row(s32 y) const;

        nodisc P & at(ivec2 p) const;
        nodisc P & at(s32 x, s32 y) const;

        void fill(const P & color) const requires (!std::is_const_v<P>);

        void outline(u32 thickness, const P & color) const requires (!std::is_const_v<P>);

        void horizontalLine(ivec2 pos, u32 length, const P & color) const requires (!std::is_const_v<P>);

        void verticalLine(ivec2 pos, u32 length, const P & color) const requires (!std::is_const_v<P>);

        void checkerboard(u32 squareSize, const P & backColor, const P & foreColor) const requires (!std::is_const_v<P>);

        void copy(const ImageView<const P> & src) const requires (!std::is_const_v<P>);
        void copy(const _Image & src) const requires (!std::is_const_v<P>);

      private:

        _Image * _image{};
        ivec2 _pos{};
        uivec2 _size{};
    };

    using GrayImageView = ImageView<u8>;
    using GrayAlphaImageView = ImageView<ucvec2>;
    using RgbImageView = ImageView<ucvec3>;
    using RgbaImageView = ImageView<ucvec4>;

    ///
    /// ...
    ///
    template <typename P> nodisc Result<Image<P>> read(const std::filesystem::path & file, bool allowComponentPadding);
    nodisc Result<GrayImage> readGrayImage(const std::filesystem::path & file);
    nodisc Result<GrayAlphaImage> readGrayAlphaImage(const std::filesystem::path & file, bool allowComponentPadding);
    nodisc Result<RgbImage> readRgbImage(const std::filesystem::path & file, bool allowComponentPadding);
    nodisc Result<RgbaImage> readRgbaImage(const std::filesystem::path & file, bool allowComponentPadding);

    template <typename P> nodisc bool write(const Image<P> & image, const std::filesystem::path & file);
}

// INLINE IMPLEMENTATION ///////////////////////////////////////////////////////////////////////////////////////////////

namespace qc::image
{
    template <typename P>
    inline Image<P>::Image(const uivec2 size) :
        Image{size.x, size.y}
    {}

    template <typename P>
    inline Image<P>::Image(const uivec2 size, P * const pixels) :
        _size{size},
        _pixels{pixels}
    {}

    template <typename P>
    inline Image<P>::Image(const u32 width, const u32 height) :
        Image{width, height, static_cast<P *>(::operator new(width * height * sizeof(P)))}
    {}

    template <typename P>
    inline Image<P>::Image(const u32 width, const u32 height, P * const pixels) :
        Image{uivec2{width, height}, pixels}
    {}

    template <typename P>
    inline Image<P>::Image(Image && other) :
        _size{other._size},
        _pixels{other._pixels}
    {
        other._size = {};
        other._pixels = nullptr;
    }

    template <typename P>
    inline Image<P> & Image<P>::operator=(Image && other)
    {
        _size = other._size;
        _pixels = other._pixels;
        other._size = {};
        other._pixels = nullptr;
        return *this;
    }

    template <typename P>
    inline Image<P>::~Image()
    {
        ::operator delete(_pixels);
    }

    template <typename P>
    inline ImageView<P> Image<P>::view()
    {
        return ImageView<P>{*this, ivec2{}, _size};
    }

    template <typename P>
    inline ImageView<const P> Image<P>::view() const
    {
        return ImageView<const P>{*this, ivec2{}, _size};
    }

    template <typename P>
    inline ImageView<P> Image<P>::view(const ivec2 pos, const uivec2 size)
    {
        return ImageView<P>{*this, pos, size};
    }

    template <typename P>
    inline ImageView<const P> Image<P>::view(const ivec2 pos, const uivec2 size) const
    {
        return ImageView<const P>{*this, pos, size};
    }

    template <typename P>
    inline P * Image<P>::row(const s32 y)
    {
        assert(y >= 0 && u32(y) < _size.y);

        return _pixels + (_size.y - 1u - u32(y)) * _size.x;
    }

    template <typename P>
    inline const P * Image<P>::row(const s32 y) const
    {
        assert(y >= 0 && u32(y) < _size.y);

        return _pixels + (_size.y - 1u - u32(y)) * _size.x;
    }

    template <typename P>
    inline P & Image<P>::at(const ivec2 p)
    {
        return at(p.x, p.y);
    }

    template <typename P>
    inline const P & Image<P>::at(const ivec2 p) const
    {
        return at(p.x, p.y);
    }

    template <typename P>
    inline P & Image<P>::at(const s32 x, const s32 y)
    {
        assert(x >= 0 && u32(x) < _size.x);

        return row(y)[x];
    }

    template <typename P>
    inline const P & Image<P>::at(const s32 x, const s32 y) const
    {
        assert(x >= 0 && u32(x) < _size.x);

        return row(y)[x];
    }

    template <typename P>
    inline ImageView<P>::ImageView(const ImageView<std::remove_const_t<P>> & other) requires std::is_const_v<P> :
        ImageView{reinterpret_cast<const ImageView &>(other)}
    {}

    template <typename P>
    inline ImageView<P> & ImageView<P>::operator=(const ImageView<std::remove_const_t<P>> & other) requires std::is_const_v<P>
    {
        return *this = reinterpret_cast<const ImageView &>(other);
    }

    template <typename P>
    inline ImageView<P>::ImageView(_Image & image, const ivec2 pos, const uivec2 size) :
        _image{&image},
        _pos{pos},
        _size{size}
    {}

    template <typename P>
    inline ImageView<P> ImageView<P>::view(const ivec2 pos, const uivec2 size) const
    {
        const ispan2 span{ispan2{_pos, _pos + ivec2(_size)} & ispan2{pos, pos + ivec2(size)}};
        return ImageView{*_image, span.min, uivec2(span.size())};
    }

    template <typename P>
    inline P * ImageView<P>::row(const s32 y) const
    {
        return _image->row(_pos.y + y) + _pos.x;
    }

    template <typename P>
    inline P & ImageView<P>::at(const ivec2 p) const
    {
        return at(p.x, p.y);
    }

    template <typename P>
    inline P & ImageView<P>::at(const s32 x, const s32 y) const
    {
        assert(x >= 0 && u32(x) < _size.x);

        return row(y)[x];
    }
}
