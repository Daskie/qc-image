#include <qc-image/image.hpp>

namespace qc::image
{
    static void * _realloc(void * const oldPtr, const size_t oldSize, const size_t newSize)
    {
        void * const newPtr{::operator new(newSize)};
        memcpy(newPtr, oldPtr, oldSize);
        operator delete(oldPtr);
        return newPtr;
    }
}

#pragma warning(push)
#pragma warning(disable: 4365 4711 4738 5219 6951)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MALLOC(size) ::operator new(size)
#define STBI_FREE(ptr) ::operator delete(ptr)
#define STBI_REALLOC_SIZED(oldPtr, oldSize, newSize) ::qc::image::_realloc(oldPtr, oldSize, newSize)
#include <stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_MALLOC STBI_MALLOC
#define STBIW_FREE STBI_FREE
#define STBIW_REALLOC_SIZED STBI_REALLOC_SIZED
#include <stb/stb_image_write.h>
#pragma warning(pop)

#include <qc-core/span-ext.hpp>

namespace qc::image
{
    template <typename P>
    void Image<P>::fill(const P & color) noexcept
    {
        std::fill_n(_pixels, _size.x * _size.y, color);
    }

    template <typename P>
    P * Image<P>::release() noexcept
    {
        _size = {};
        return std::exchange(_pixels, nullptr);
    }

    template <typename P>
    void ImageView<P>::fill(const P & color) const noexcept requires (!std::is_const_v<P>)
    {
        for (int y{0}; y < _size.y; ++y)
        {
            std::fill_n(row(y), _size.x, color);
        }
    }

    template <typename P>
    void ImageView<P>::outline(const int thickness, const P & color) const noexcept requires (!std::is_const_v<P>)
    {
        if (thickness)
        {
            horizontalLine({0, 0}, _size.x, color);
            horizontalLine({0, _size.y - 1}, _size.x, color);
            verticalLine({0, 1}, _size.y - 2, color);
            verticalLine({_size.x - 1, 1}, _size.y - 2, color);

            view(ivec2{1}, _size - 2).outline(thickness - 1, color);
        }
    }

    template <typename P>
    void ImageView<P>::horizontalLine(const ivec2 pos, const int length, const P & color) const noexcept requires (!std::is_const_v<P>)
    {
        if (length > 0 && pos.y >= 0 && pos.y < _size.y)
        {
            P * const r{row(pos.y)};
            std::fill(r + max(pos.x, 0), r + min(pos.x + length, _size.x), color);
        }
    }

    template <typename P>
    void ImageView<P>::verticalLine(const ivec2 pos, const int length, const P & color) const noexcept requires (!std::is_const_v<P>)
    {
        if (length > 0 && pos.x >= 0 && pos.x < _size.x)
        {
            const int startY{max(pos.y, 0)};
            const int endY{min(pos.y + length, _size.y)};
            P * p{&at(pos.x, startY)};
            for (int y{startY}; y < endY; ++y, p -= _image->_size.x)
            {
                *p = color;
            }
        }
    }

    template <typename P>
    void ImageView<P>::checkerboard(const int squareSize, const P & backColor, const P & foreColor) const noexcept requires (!std::is_const_v<P>)
    {
        // TODO: Make more efficient
        for (ivec2 p{0, 0}; p.y < _size.y; ++p.y)
        {
            for (p.x = 0; p.x < _size.x; ++p.x)
            {
                at(p) = (p.x / squareSize + p.y / squareSize) % 2 ? foreColor : backColor;
            }
        }
    }

    template <typename P>
    void ImageView<P>::copy(const ImageView<const P> & src) const noexcept requires (!std::is_const_v<P>)
    {
        assert(_size == src._size);

        const P * srcR{src.row(0)};
        P * dstR{row(0)};
        for (int y{0}; y < _size.y; ++y, srcR -= src._image->_size.x, dstR -= _image->_size.x)
        {
            std::copy_n(srcR, _size.x, dstR);
        }
    }

    template <typename P>
    void ImageView<P>::copy(const Image & src) const noexcept requires (!std::is_const_v<P>)
    {
        copy(src.view());
    }

    template <typename P>
    Image<P> read(const std::filesystem::path & file, const bool allowComponentPadding)
    {
        const std::filesystem::file_status fileStatus{std::filesystem::status(file)};
        if (fileStatus.type() != std::filesystem::file_type::regular)
        {
            throw std::exception{};
        }

        int x, y, channels;
        u8 * data{stbi_load(file.string().c_str(), &x, &y, &channels, allowComponentPadding ? Image<P>::components : 0)};

        if (channels > Image<P>::components || !allowComponentPadding && channels < Image<P>::components)
        {
            throw std::exception{};
        }

        return Image<P>{ivec2{x, y}, reinterpret_cast<P *>(data)};
    }

    GrayImage readGrayImage(const std::filesystem::path & file)
    {
        return read<u8>(file, false);
    }

    GrayAlphaImage readGrayAlphaImage(const std::filesystem::path & file, const bool allowComponentPadding)
    {
        return read<ucvec2>(file, allowComponentPadding);
    }

    RgbImage readRgbImage(const std::filesystem::path & file, const bool allowComponentPadding)
    {
        return read<ucvec3>(file, allowComponentPadding);
    }

    RgbaImage readRgbaImage(const std::filesystem::path & file, const bool allowComponentPadding)
    {
        return read<ucvec4>(file, allowComponentPadding);
    }

    template <typename P>
    void write(const Image<P> & image, const std::filesystem::path & file)
    {
        const std::filesystem::path extension{file.extension()};
        if (extension == ".png")
        {
            stbi_write_png(file.string().c_str(), image.size().x, image.size().y, image.components, image.pixels(), image.size().x * int(sizeof(P)));
        }
        else
        {
            throw std::exception{}; // Currently unsupported
        }
    }

    // Explicit template specialization

    template class Image<u8>;
    template class Image<ucvec2>;
    template class Image<ucvec3>;
    template class Image<ucvec4>;

    template class ImageView<u8>;
    template class ImageView<ucvec2>;
    template class ImageView<ucvec3>;
    template class ImageView<ucvec4>;

    template GrayImage read<u8>(const std::filesystem::path &, bool);
    template GrayAlphaImage read<ucvec2>(const std::filesystem::path &, bool);
    template RgbImage read<ucvec3>(const std::filesystem::path &, bool);
    template RgbaImage read<ucvec4>(const std::filesystem::path &, bool);

    template void write(const GrayImage &, const std::filesystem::path &);
    template void write(const GrayAlphaImage &, const std::filesystem::path &);
    template void write(const RgbImage &, const std::filesystem::path &);
    template void write(const RgbaImage &, const std::filesystem::path &);
}
