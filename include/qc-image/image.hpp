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
    template <typename P> class ImageView;

    template <typename P>
    class Image
    {
        friend class ImageView<P>;
        friend class ImageView<const P>;

      public:

        static constexpr int components{sizeof(P)};

        Image() = default;
        explicit Image(ivec2 size);
        Image(ivec2 size, P * pixels);
        Image(int width, int height);
        Image(int width, int height, P * pixels);

        Image(const Image &) = delete;
        Image(Image && other);

        Image & operator=(const Image &) = delete;
        Image & operator=(Image && other);

        ~Image();

        void fill(const P & color);

        nodisc ImageView<P> view();
        nodisc ImageView<const P> view() const;
        nodisc ImageView<P> view(const ispan2 & span);
        nodisc ImageView<const P> view(const ispan2 & span) const;
        nodisc ImageView<P> view(ivec2 position, ivec2 size);
        nodisc ImageView<const P> view(ivec2 position, ivec2 size) const;

        nodisc ivec2 size() const { return _size; }

        nodisc int width() const { return _size.x; };

        nodisc int height() const { return _size.y; };

        nodisc P * pixels() { return _pixels; };
        nodisc const P * pixels() const { return _pixels; };

        nodisc P * row(int y);
        nodisc const P * row(int y) const;

        nodisc P & at(ivec2 p);
        nodisc const P & at(ivec2 p) const;
        nodisc P & at(int x, int y);
        nodisc const P & at(int x, int y) const;

        P * release();

      private:

        ivec2 _size{};
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
        friend class Image<_ImageP>;
        friend class ImageView<std::remove_const_t<P>>;
        friend class ImageView<std::add_const_t<P>>;

      public:

        using Image = std::conditional_t<std::is_const_v<P>, const Image<_ImageP>, Image<_ImageP>>;

        ImageView() = default;

        ImageView(const ImageView &) = default;
        ImageView(const ImageView<std::remove_const_t<P>> & other) requires std::is_const_v<P>;

        ImageView & operator=(const ImageView &) = default;
        ImageView & operator=(const ImageView<std::remove_const_t<P>> & other) requires std::is_const_v<P>;

        nodisc ImageView view(const ispan2 & span) const;
        nodisc ImageView view(ivec2 position, ivec2 size) const;

        nodisc Image * image() const { return _image; }

        nodisc const ispan2 & span() const { return _span; }

        nodisc ivec2 pos() const { return _span.min; }

        nodisc ivec2 size() const { return _size; }

        nodisc int width() const { return _size.x; }

        nodisc int height() const { return _size.y; }

        nodisc P * row(int y) const;

        nodisc P & at(ivec2 p) const;
        nodisc P & at(int x, int y) const;

        void fill(const P & color) const requires (!std::is_const_v<P>);

        void outline(int thickness, const P & color) const requires (!std::is_const_v<P>);

        void horizontalLine(ivec2 pos, int length, const P & color) const requires (!std::is_const_v<P>);

        void verticalLine(ivec2 pos, int length, const P & color) const requires (!std::is_const_v<P>);

        void checkerboard(int squareSize, const P & backColor, const P & foreColor) const requires (!std::is_const_v<P>);

        void copy(const ImageView<const P> & src) const requires (!std::is_const_v<P>);
        void copy(const Image & src) const requires (!std::is_const_v<P>);

      private:

        Image * _image{};
        ispan2 _span{};
        ivec2 _size{};

        ImageView(Image & image, const ispan2 & span);
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
    inline Image<P>::Image(const ivec2 size) :
        Image(size.x, size.y)
    {}

    template <typename P>
    inline Image<P>::Image(const ivec2 size, P * const pixels) :
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
        return ImageView<P>{*this, ispan2{0, _size}};
    }

    template <typename P>
    inline ImageView<const P> Image<P>::view() const
    {
        return ImageView<const P>{*this, ispan2{0, _size}};
    }

    template <typename P>
    inline ImageView<P> Image<P>::view(const ispan2 & span)
    {
        return ImageView<P>{*this, clamp(span, ivec2{0}, _size)};
    }

    template <typename P>
    inline ImageView<const P> Image<P>::view(const ispan2 & span) const
    {
        return ImageView<const P>{*this, clamp(span, ivec2{0}, _size)};
    }

    template <typename P>
    inline ImageView<P> Image<P>::view(const ivec2 pos, const ivec2 size)
    {
        return ImageView<P>{*this, ispan2{pos, pos + size}};
    }

    template <typename P>
    inline ImageView<const P> Image<P>::view(const ivec2 pos, const ivec2 size) const
    {
        return ImageView<const P>{*this, ispan2{pos, pos + size}};
    }

    template <typename P>
    inline P * Image<P>::row(const int y)
    {
        assert(y >= 0 && y < _size.y);

        return _pixels + (_size.y - 1 - y) * _size.x;
    }

    template <typename P>
    inline const P * Image<P>::row(const int y) const
    {
        assert(y >= 0 && y < _size.y);

        return _pixels + (_size.y - 1 - y) * _size.x;
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
    inline P & Image<P>::at(const int x, const int y)
    {
        assert(x >= 0 && x < _size.x);

        return row(y)[x];
    }

    template <typename P>
    inline const P & Image<P>::at(const int x, const int y) const
    {
        assert(x >= 0 && x < _size.x);

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
    inline ImageView<P>::ImageView(Image & image, const ispan2 & span) :
        _image{&image},
        _span{span},
        _size{_span.size()}
    {}

    template <typename P>
    inline ImageView<P> ImageView<P>::view(const ispan2 & span) const
    {
        return ImageView{*_image, span & _span};
    }

    template <typename P>
    inline ImageView<P> ImageView<P>::view(const ivec2 pos, const ivec2 size) const
    {
        return view(ispan2{pos, pos + size});
    }

    template <typename P>
    inline P * ImageView<P>::row(const int y) const
    {
        return _image->row(_span.min.y + y) + _span.min.x;
    }

    template <typename P>
    inline P & ImageView<P>::at(const ivec2 p) const
    {
        return at(p.x, p.y);
    }

    template <typename P>
    inline P & ImageView<P>::at(const int x, const int y) const
    {
        assert(x >= 0 && x < _size.x);

        return row(y)[x];
    }
}
