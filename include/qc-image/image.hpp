#pragma once

#include <cassert>

#include <filesystem>

#include <qc-core/core.hpp>
#include <qc-core/span.hpp>
#include <qc-core/vector.hpp>

namespace qc::image
{
    template <Numeric T, u32 n> using Pixel = std::conditional_t<n == 1u, T, vec<T, n>>;

    template <Numeric T, u32 n, bool constant> class ImageView;

    template <Numeric T, u32 n>
    class Image
    {
      public:

        static_assert(n >= 1u && n <= 4u);

        friend class ImageView<T, n, false>;
        friend class ImageView<T, n, true>;

        using ComponentT = T;
        using Pixel = Pixel<T, n>;
        using View = ImageView<T, n, false>;
        using CView = ImageView<T, n, true>;

        static constexpr u32 componentN{n};

        Image() = default;
        explicit Image(uivec2 size);
        Image(uivec2 size, Pixel * pixels);
        Image(u32 width, u32 height);
        Image(u32 width, u32 height, Pixel * pixels);

        Image(const Image &) = delete;
        Image(Image && other);

        Image & operator=(const Image &) = delete;
        Image & operator=(Image && other);

        ~Image();

        void fill(const Pixel & color);

        nodisc View view() { return View{*this, ivec2{}, _size}; }
        nodisc CView view() const { return CView{*this, ivec2{}, _size}; }
        nodisc View view(const ivec2 pos, const uivec2 size) { return View{*this, pos, size}; }
        nodisc CView view(const ivec2 pos, const uivec2 size) const { return CView{*this, pos, size}; }

        nodisc uivec2 size() const { return _size; }

        nodisc u32 width() const { return _size.x; };

        nodisc u32 height() const { return _size.y; };

        nodisc Pixel * pixels() { return _pixels; };
        nodisc const Pixel * pixels() const { return _pixels; };

        nodisc Pixel * row(s32 y);
        nodisc const Pixel * row(s32 y) const;

        nodisc Pixel & at(ivec2 p);
        nodisc const Pixel & at(ivec2 p) const;
        nodisc Pixel & at(s32 x, s32 y);
        nodisc const Pixel & at(s32 x, s32 y) const;

        Pixel * release();

      private:

        uivec2 _size{};
        Pixel * _pixels{};
    };

    using GrayImage = Image<u8, 1u>;
    using GrayAlphaImage = Image<u8, 2u>;
    using RgbImage = Image<u8, 3u>;
    using RgbaImage = Image<u8, 4u>;

    template <Numeric T, u32 n, bool constant>
    class ImageView
    {
      public:

        friend class Image<T, n>;
        friend class ImageView<T, n, !constant>;

        using Image = ConstIf<Image<T, n>, constant>;
        using Pixel = ConstIf<Pixel<T, n>, constant>;

      public:

        ImageView() = default;
        ImageView(Image & image, ivec2 pos, uivec2 size);

        ImageView(const ImageView &) = default;
        ImageView(const ImageView<T, n, false> & other) requires (constant);

        ImageView & operator=(const ImageView &) = default;
        ImageView & operator=(const ImageView<T, n, false> & other) requires (constant);

        nodisc ImageView view(ivec2 position, uivec2 size) const;

        nodisc Image * image() const { return _image; }

        nodisc ivec2 pos() const { return _pos; }

        nodisc uivec2 size() const { return _size; }

        nodisc u32 width() const { return _size.x; }

        nodisc u32 height() const { return _size.y; }

        nodisc Pixel * row(s32 y) const;

        nodisc Pixel & at(ivec2 p) const;
        nodisc Pixel & at(s32 x, s32 y) const;

        void fill(const Pixel & color) const requires (!constant);

        void outline(u32 thickness, const Pixel & color) const requires (!constant);

        void horizontalLine(ivec2 pos, u32 length, const Pixel & color) const requires (!constant);

        void verticalLine(ivec2 pos, u32 length, const Pixel & color) const requires (!constant);

        void checkerboard(u32 squareSize, const Pixel & backColor, const Pixel & foreColor) const requires (!constant);

        void copy(const ImageView<T, n, true> & src) const requires (!constant);
        void copy(const Image & src) const requires (!constant);

      private:

        Image * _image{};
        ivec2 _pos{};
        uivec2 _size{};
    };

    ///
    /// ...
    ///
    template <Numeric T, u32 n> nodisc Result<Image<T, n>> read(const std::filesystem::path & file, bool allowComponentPadding);
    nodisc Result<GrayImage> readGray(const std::filesystem::path & file);
    nodisc Result<GrayAlphaImage> readGrayAlpha(const std::filesystem::path & file, bool allowComponentPadding);
    nodisc Result<RgbImage> readRgb(const std::filesystem::path & file, bool allowComponentPadding);
    nodisc Result<RgbaImage> readRgba(const std::filesystem::path & file, bool allowComponentPadding);

    template <Numeric T, u32 n> nodisc bool write(const Image<T, n> & image, const std::filesystem::path & file);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace qc::image
{
    template <Numeric T, u32 n>
    inline Image<T, n>::Image(const uivec2 size) :
        Image{size.x, size.y}
    {}

    template <Numeric T, u32 n>
    inline Image<T, n>::Image(const uivec2 size, Pixel * const pixels) :
        _size{size},
        _pixels{pixels}
    {}

    template <Numeric T, u32 n>
    inline Image<T, n>::Image(const u32 width, const u32 height) :
        Image{width, height, static_cast<Pixel *>(::operator new(width * height * sizeof(Pixel)))}
    {}

    template <Numeric T, u32 n>
    inline Image<T, n>::Image(const u32 width, const u32 height, Pixel * const pixels) :
        Image{uivec2{width, height}, pixels}
    {}

    template <Numeric T, u32 n>
    inline Image<T, n>::Image(Image && other) :
        _size{other._size},
        _pixels{other._pixels}
    {
        other._size = {};
        other._pixels = nullptr;
    }

    template <Numeric T, u32 n>
    inline Image<T, n> & Image<T, n>::operator=(Image && other)
    {
        _size = other._size;
        _pixels = other._pixels;
        other._size = {};
        other._pixels = nullptr;
        return *this;
    }

    template <Numeric T, u32 n>
    inline Image<T, n>::~Image()
    {
        ::operator delete(_pixels);
    }

    template <Numeric T, u32 n>
    inline auto Image<T, n>::row(const s32 y) -> Pixel *
    {
        assert(y >= 0 && u32(y) < _size.y);

        return _pixels + (_size.y - 1u - u32(y)) * _size.x;
    }

    template <Numeric T, u32 n>
    inline auto Image<T, n>::row(const s32 y) const -> const Pixel *
    {
        assert(y >= 0 && u32(y) < _size.y);

        return _pixels + (_size.y - 1u - u32(y)) * _size.x;
    }

    template <Numeric T, u32 n>
    inline auto Image<T, n>::at(const ivec2 p) -> Pixel &
    {
        return at(p.x, p.y);
    }

    template <Numeric T, u32 n>
    inline auto Image<T, n>::at(const ivec2 p) const -> const Pixel &
    {
        return at(p.x, p.y);
    }

    template <Numeric T, u32 n>
    inline auto Image<T, n>::at(const s32 x, const s32 y) -> Pixel &
    {
        assert(x >= 0 && u32(x) < _size.x);

        return row(y)[x];
    }

    template <Numeric T, u32 n>
    inline auto Image<T, n>::at(const s32 x, const s32 y) const -> const Pixel &
    {
        assert(x >= 0 && u32(x) < _size.x);

        return row(y)[x];
    }

    template <Numeric T, u32 n, bool constant>
    inline ImageView<T, n, constant>::ImageView(const ImageView<T, n, false> & other) requires (constant) :
        ImageView{reinterpret_cast<const ImageView &>(other)}
    {}

    template <Numeric T, u32 n, bool constant>
    inline auto ImageView<T, n, constant>::operator=(const ImageView<T, n, false> & other) -> ImageView & requires (constant)
    {
        return *this = reinterpret_cast<const ImageView &>(other);
    }

    template <Numeric T, u32 n, bool constant>
    inline ImageView<T, n, constant>::ImageView(Image & image, const ivec2 pos, const uivec2 size) :
        _image{&image},
        _pos{pos},
        _size{size}
    {}

    template <Numeric T, u32 n, bool constant>
    inline auto ImageView<T, n, constant>::view(const ivec2 pos, const uivec2 size) const -> ImageView
    {
        const ispan2 span{ispan2{_pos, _pos + ivec2(_size)} & ispan2{pos, pos + ivec2(size)}};
        return ImageView{*_image, span.min, uivec2(span.size())};
    }

    template <Numeric T, u32 n, bool constant>
    inline auto ImageView<T, n, constant>::row(const s32 y) const -> Pixel *
    {
        return _image->row(_pos.y + y) + _pos.x;
    }

    template <Numeric T, u32 n, bool constant>
    inline auto ImageView<T, n, constant>::at(const ivec2 p) const -> Pixel &
    {
        return at(p.x, p.y);
    }

    template <Numeric T, u32 n, bool constant>
    inline auto ImageView<T, n, constant>::at(const s32 x, const s32 y) const -> Pixel &
    {
        assert(x >= 0 && u32(x) < _size.x);

        return row(y)[x];
    }
}
