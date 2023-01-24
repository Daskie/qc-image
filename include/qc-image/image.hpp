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

        Image() noexcept = default;
        explicit Image(ivec2 size);
        Image(ivec2 size, P * pixels);
        Image(int width, int height);
        Image(int width, int height, P * pixels);

        Image(const Image &) = delete;
        Image(Image && other) noexcept;

        Image & operator=(const Image &) = delete;
        Image & operator=(Image && other) noexcept;

        ~Image() noexcept;

        void fill(const P & color) noexcept;

        ImageView<P> view() noexcept;
        ImageView<const P> view() const noexcept;
        ImageView<P> view(const ispan2 & span) noexcept;
        ImageView<const P> view(const ispan2 & span) const noexcept;
        ImageView<P> view(ivec2 position, ivec2 size) noexcept;
        ImageView<const P> view(ivec2 position, ivec2 size) const noexcept;

        ivec2 size() const noexcept { return _size; }

        int width() const noexcept { return _size.x; };

        int height() const noexcept { return _size.y; };

        P * pixels() noexcept { return _pixels; };
        const P * pixels() const noexcept { return _pixels; };

        P * row(int y) noexcept;
        const P * row(int y) const noexcept;

        P & at(ivec2 p) noexcept;
        const P & at(ivec2 p) const noexcept;
        P & at(int x, int y) noexcept;
        const P & at(int x, int y) const noexcept;

        P * release() noexcept;

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

        ImageView() noexcept = default;

        ImageView(const ImageView &) noexcept = default;
        ImageView(const ImageView<std::remove_const_t<P>> & other) noexcept requires std::is_const_v<P>;

        ImageView & operator=(const ImageView &) noexcept = default;
        ImageView & operator=(const ImageView<std::remove_const_t<P>> & other) noexcept requires std::is_const_v<P>;

        ImageView view(const ispan2 & span) const noexcept;
        ImageView view(ivec2 position, ivec2 size) const noexcept;

        Image * image() const noexcept { return _image; }

        const ispan2 & span() const noexcept { return _span; }

        ivec2 pos() const noexcept { return _span.min; }

        ivec2 size() const noexcept { return _size; }

        int width() const noexcept { return _size.x; }

        int height() const noexcept { return _size.y; }

        P * row(int y) const noexcept;

        P & at(ivec2 p) const noexcept;
        P & at(int x, int y) const noexcept;

        void fill(const P & color) const noexcept requires (!std::is_const_v<P>);

        void outline(const P & color) const noexcept requires (!std::is_const_v<P>);

        void horizontalLine(ivec2 pos, int length, const P & color) const noexcept requires (!std::is_const_v<P>);

        void verticalLine(ivec2 pos, int length, const P & color) const noexcept requires (!std::is_const_v<P>);

        void checkerboard(int squareSize, const P & backColor, const P & foreColor) const noexcept requires (!std::is_const_v<P>);

        void copy(const ImageView<const P> & src) const noexcept requires (!std::is_const_v<P>);
        void copy(const Image & src) const noexcept requires (!std::is_const_v<P>);

      private:

        Image * _image{};
        ispan2 _span{};
        ivec2 _size{};

        ImageView(Image & image, const ispan2 & span) noexcept;
    };

    using GrayImageView = ImageView<u8>;
    using GrayAlphaImageView = ImageView<ucvec2>;
    using RgbImageView = ImageView<ucvec3>;
    using RgbaImageView = ImageView<ucvec4>;

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
    inline ImageView<P> Image<P>::view() noexcept
    {
        return ImageView<P>{*this, ispan2{0, _size}};
    }

    template <typename P>
    inline ImageView<const P> Image<P>::view() const noexcept
    {
        return ImageView<const P>{*this, ispan2{0, _size}};
    }

    template <typename P>
    inline ImageView<P> Image<P>::view(const ispan2 & span) noexcept
    {
        return ImageView<P>{*this, clamp(span, ivec2{0}, _size)};
    }

    template <typename P>
    inline ImageView<const P> Image<P>::view(const ispan2 & span) const noexcept
    {
        return ImageView<const P>{*this, clamp(span, ivec2{0}, _size)};
    }

    template <typename P>
    inline ImageView<P> Image<P>::view(const ivec2 pos, const ivec2 size) noexcept
    {
        return ImageView<P>{*this, ispan2{pos, pos + size}};
    }

    template <typename P>
    inline ImageView<const P> Image<P>::view(const ivec2 pos, const ivec2 size) const noexcept
    {
        return ImageView<const P>{*this, ispan2{pos, pos + size}};
    }

    template <typename P>
    inline P * Image<P>::row(const int y) noexcept
    {
        return _pixels + (_size.y - 1 - y) * _size.x;
    }

    template <typename P>
    inline const P * Image<P>::row(const int y) const noexcept
    {
        return _pixels + (_size.y - 1 - y) * _size.x;
    }

    template <typename P>
    inline P & Image<P>::at(const ivec2 p) noexcept
    {
        return at(p.x, p.y);
    }

    template <typename P>
    inline const P & Image<P>::at(const ivec2 p) const noexcept
    {
        return at(p.x, p.y);
    }

    template <typename P>
    inline P & Image<P>::at(const int x, const int y) noexcept
    {
        assert(x >= 0 && x < _size.x && y >= 0 && y < _size.y);

        return row(y)[x];
    }

    template <typename P>
    inline const P & Image<P>::at(const int x, const int y) const noexcept
    {
        assert(x >= 0 && x < _size.x && y >= 0 && y < _size.y);

        return row(y)[x];
    }

    template <typename P>
    inline ImageView<P>::ImageView(const ImageView<std::remove_const_t<P>> & other) noexcept requires std::is_const_v<P> :
        ImageView{reinterpret_cast<const ImageView &>(other)}
    {}

    template <typename P>
    inline ImageView<P> & ImageView<P>::operator=(const ImageView<std::remove_const_t<P>> & other) noexcept requires std::is_const_v<P>
    {
        return *this = reinterpret_cast<const ImageView &>(other);
    }

    template <typename P>
    inline ImageView<P>::ImageView(Image & image, const ispan2 & span) noexcept :
        _image{&image},
        _span{span},
        _size{_span.size()}
    {}

    template <typename P>
    inline ImageView<P> ImageView<P>::view(const ispan2 & span) const noexcept
    {
        return ImageView{*_image, span & _span};
    }

    template <typename P>
    inline ImageView<P> ImageView<P>::view(const ivec2 pos, const ivec2 size) const noexcept
    {
        return view(ispan2{pos, pos + size});
    }

    template <typename P>
    inline P * ImageView<P>::row(const int y) const noexcept
    {
        return _image->row(_span.min.y + y) + _span.min.x;
    }

    template <typename P>
    inline P & ImageView<P>::at(const ivec2 p) const noexcept
    {
        return at(p.x, p.y);
    }

    template <typename P>
    inline P & ImageView<P>::at(const int x, const int y) const noexcept
    {
        assert(x >= 0 && x < _size.x && y >= 0 && y < _size.y);

        return row(y)[x];
    }
}
