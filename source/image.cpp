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
        const ispan2 trueRegion{qc::intersect(ispan2{{}, _size}, region)};
        const ivec2 trueSize{trueRegion.size()};

        for (ivec2 p{trueRegion.min}; p.y < trueRegion.max.y; ++p.y) {
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
        for (int x{region.min.x}; x < region.max.x; ++x) {
            at(x, region.min.y) = color;
            at(x, region.max.y - 1) = color;
        }
        for (int y{region.min.y}; y < region.max.y; ++y) {
            at(region.min.x, y) = color;
            at(region.max.x - 1, y) = color;
        }
    }

    template <typename P>
    void Image<P>::outline(const ivec2 & pos, const ivec2 & size, const P & color) noexcept
    {
        outline(ispan2{pos, pos + size}, color);
    }

    template <typename P>
    void Image<P>::copy(const Image & src, const ivec2 & dstPos) noexcept
    {
        copy(src, {{}, src.size()}, dstPos);
    }

    template <typename P>
    void Image<P>::copy(const Image & src, const ispan2 & srcRegion, const ivec2 & dstPos) noexcept
    {
        const ispan2 trueDstRegion{qc::intersect(ispan2{{}, _size}, dstPos + srcRegion)};
        const ivec2 trueSrcPos{trueDstRegion.min - dstPos};
        const ivec2 trueSize{trueDstRegion.size()};

        for (ivec2 srcP{trueSrcPos}, dstP{trueDstRegion.min}; dstP.y < trueDstRegion.max.y; ++srcP.y, ++dstP.y) {
            std::copy_n(&src.at(srcP), trueSize.x, &at(dstP));
        }
    }

    template <typename P>
    Image<P> read(const std::filesystem::path & file)
    {
        const std::filesystem::file_status fileStatus{std::filesystem::status(file)};
        if (fileStatus.type() != std::filesystem::file_type::regular) {
            throw std::exception{};
        }

        int x, y, channels;
        u8 * data{stbi_load(file.string().c_str(), &x, &y, &channels, 0)};

        if (channels != Image<P>::components) {
            throw std::exception{};
        }

        return Image<P>{ivec2{x, y}, reinterpret_cast<P *>(data)};
    }

    template <typename P>
    void write(const Image<P> & image, const std::filesystem::path & file)
    {
        const std::filesystem::path extension{file.extension()};
        if (extension == ".png") {
            stbi_write_png(file.string().c_str(), image.size().x, image.size().y, image.components, image.pixels(), image.size().x * sizeof(P));
        }
        else {
            throw std::exception{}; // Currently unsupported
        }
    }

    // Explicit template specialization

    template class Image<u8>;
    template class Image<ucvec2>;
    template class Image<ucvec3>;
    template class Image<ucvec4>;

    template GrayImage read<u8>(const std::filesystem::path &);
    template GrayAlphaImage read<ucvec2>(const std::filesystem::path &);
    template RgbImage read<ucvec3>(const std::filesystem::path &);
    template RgbaImage read<ucvec4>(const std::filesystem::path &);

    template void write(const GrayImage &, const std::filesystem::path &);
    template void write(const GrayAlphaImage &, const std::filesystem::path &);
    template void write(const RgbImage &, const std::filesystem::path &);
    template void write(const RgbaImage &, const std::filesystem::path &);
}
