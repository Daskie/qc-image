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
    void Image<P>::fill(const ispan2 & region, const P & color) noexcept
    {
        const ispan2 trueRegion{ispan2{{}, _size} & region};
        const ivec2 trueSize{trueRegion.size()};

        for (ivec2 p{trueRegion.min}; p.y < trueRegion.max.y; ++p.y)
        {
            std::fill_n(&at(p), trueSize.x, color);
        }
    }

    template <typename P>
    void Image<P>::fill(const ivec2 & pos, const ivec2 & size, const P & color) noexcept
    {
        fill(ispan2{pos, pos + size}, color);
    }

    template <typename P>
    void Image<P>::outline(const ispan2 & region, const P & color) noexcept
    {
        const ivec2 size{region.size()};
        horizontalLine(region.min, size.x, color);
        horizontalLine(ivec2{region.min.x, region.max.y - 1}, size.x, color);
        verticalLine(ivec2{region.min.x, region.min.y + 1}, size.y - 2, color);
        verticalLine(ivec2{region.max.x - 1, region.min.y + 1}, size.y - 2, color);
    }

    template <typename P>
    void Image<P>::outline(const ivec2 & pos, const ivec2 & size, const P & color) noexcept
    {
        outline(ispan2{pos, pos + size}, color);
    }

    template <typename P>
    void Image<P>::horizontalLine(const ivec2 & pos, const int length, const P & color) noexcept
    {
        if (pos.y >= 0 && pos.y < _size.y)
        {
            for (int x{qc::max(pos.x, 0)}, endX{qc::min(pos.x + length, _size.x)}; x < endX; ++x)
            {
                at(x, pos.y) = color;
            }
        }
    }

    template <typename P>
    void Image<P>::verticalLine(const ivec2 & pos, const int length, const P & color) noexcept
    {
        if (pos.x >= 0 && pos.x < _size.x)
        {
            for (int y{qc::max(pos.y, 0)}, endY{qc::min(pos.y + length, _size.y)}; y < endY; ++y)
            {
                at(pos.x, y) = color;
            }
        }
    }

    template <typename P>
    void Image<P>::diagonalLine(const ivec2 & pos, const int length, const int thickness, const bool slope, const P & color) noexcept
    {
        if (slope)
        {
            for (int i{0}; i < length; ++i)
            {
                at(pos + i) = color;
            }

            for (int j{1}; j < thickness; ++j)
            {
                for (int i{0}; i < length - j; ++i)
                {
                    at(pos.x + j + i, pos.y + i) = color;
                    at(pos.x + i, pos.y + j + i) = color;
                }
            }
        }
        else
        {
            for (int i{0}; i < length; ++i)
            {
                at(pos.x + i, pos.y + length - 1 - i) = color;
            }

            for (int j{1}; j < thickness; ++j)
            {
                for (int i{0}; i < length - j; ++i)
                {
                    at(pos.x + j + i, pos.y + length - 1 - i) = color;
                    at(pos.x + i, pos.y + length - 1 - j - i) = color;
                }
            }
        }
    }

    template <typename P>
    void Image<P>::checkerboard(const int squareSize, const P & backColor, const P & foreColor) noexcept
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
    void Image<P>::copy(const Image & src, const ivec2 & dstPos) noexcept
    {
        copy(src, {{}, src.size()}, dstPos);
    }

    template <typename P>
    void Image<P>::copy(const Image & src, const ispan2 & srcRegion, const ivec2 & dstPos) noexcept
    {
        const ispan2 trueDstRegion{ispan2{{}, _size} & dstPos + srcRegion};
        const ivec2 trueSrcPos{trueDstRegion.min - dstPos};
        const ivec2 trueSize{trueDstRegion.size()};

        for (ivec2 srcP{trueSrcPos}, dstP{trueDstRegion.min}; dstP.y < trueDstRegion.max.y; ++srcP.y, ++dstP.y)
        {
            std::copy_n(&src.at(srcP), trueSize.x, &at(dstP));
        }
    }

    template <typename P>
    P * Image<P>::release() noexcept
    {
        _size = {};
        return std::exchange(_pixels, nullptr);
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

    template GrayImage read<u8>(const std::filesystem::path &, bool);
    template GrayAlphaImage read<ucvec2>(const std::filesystem::path &, bool);
    template RgbImage read<ucvec3>(const std::filesystem::path &, bool);
    template RgbaImage read<ucvec4>(const std::filesystem::path &, bool);

    template void write(const GrayImage &, const std::filesystem::path &);
    template void write(const GrayAlphaImage &, const std::filesystem::path &);
    template void write(const RgbImage &, const std::filesystem::path &);
    template void write(const RgbaImage &, const std::filesystem::path &);
}
