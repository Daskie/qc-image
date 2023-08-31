#include <qc-image/image.hpp>

#include <qc-core/utils.hpp>

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

MSVC_WARNING_PUSH
MSVC_WARNING_DISABLE(4365 4711 4738 5219 6951)
GCC_DIAGNOSTIC_PUSH
GCC_DIAGNOSTIC_IGNORED("-Wconversion")
GCC_DIAGNOSTIC_IGNORED("-Wduplicated-branches")
GCC_DIAGNOSTIC_IGNORED("-Wmissing-field-initializers")
GCC_DIAGNOSTIC_IGNORED("-Wold-style-cast")
GCC_DIAGNOSTIC_IGNORED("-Wparentheses")
GCC_DIAGNOSTIC_IGNORED("-Wsign-compare")
GCC_DIAGNOSTIC_IGNORED("-Wsign-conversion")
GCC_DIAGNOSTIC_IGNORED("-Wuseless-cast")
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
MSVC_WARNING_POP
GCC_DIAGNOSTIC_POP

namespace qc::image
{
    template <Numeric T, u32 n>
    void Image<T, n>::fill(const Pixel & color)
    {
        std::fill_n(_pixels, _size.x * _size.y, color);
    }

    template <Numeric T, u32 n>
    auto Image<T, n>::release() -> Pixel *
    {
        Pixel * const pixels{_pixels};
        _size = {};
        _pixels = nullptr;
        return pixels;
    }

    template <Numeric T, u32 n, bool constant>
    void ImageView<T, n, constant>::fill(const Pixel & color) const requires (!constant)
    {
        for (u32 y{0u}; y < _size.y; ++y)
        {
            std::fill_n(row(s32(y)), _size.x, color);
        }
    }

    template <Numeric T, u32 n, bool constant>
    void ImageView<T, n, constant>::outline(const u32 thickness, const Pixel & color) const requires (!constant)
    {
        if (thickness)
        {
            horizontalLine({0, 0}, _size.x, color);
            if (_size.y > 1)
            {
                horizontalLine({0, s32(_size.y - 1u)}, _size.x, color);
                verticalLine({0, 1}, _size.y - 2u, color);
                verticalLine({s32(_size.x - 1u), 1}, _size.y - 2u, color);
            }

            if (thickness > 1u && min(_size) > 2u)
            {
                view(ivec2{1}, _size - 2u).outline(thickness - 1u, color);
            }
        }
    }

    template <Numeric T, u32 n, bool constant>
    void ImageView<T, n, constant>::horizontalLine(const ivec2 pos, const u32 length, const Pixel & color) const requires (!constant)
    {
        if (pos.y >= 0 && u32(pos.y) < _size.y)
        {
            const ispan1 span{ispan1{pos.x, pos.x + s32(length)} & ispan1{0, s32(_size.x)}};
            Pixel * const r{row(pos.y)};
            std::fill(r + span.min, r + span.max, color);
        }
    }

    template <Numeric T, u32 n, bool constant>
    void ImageView<T, n, constant>::verticalLine(const ivec2 pos, const u32 length, const Pixel & color) const requires (!constant)
    {
        if (pos.x >= 0 && u32(pos.x) < _size.x)
        {
            const ispan1 span{ispan1{pos.y, pos.y + s32(length)} & ispan1{0, s32(_size.y)}};
            Pixel * p{&at(pos.x, span.min)};
            for (s32 y{span.min}; y < span.max; ++y, p -= _image->_size.x)
            {
                *p = color;
            }
        }
    }

    template <Numeric T, u32 n, bool constant>
    void ImageView<T, n, constant>::checkerboard(const u32 squareSize, const Pixel & backColor, const Pixel & foreColor) const requires (!constant)
    {
        // TODO: Make more efficient
        for (uivec2 p{0u}; p.y < _size.y; ++p.y)
        {
            for (p.x = 0u; p.x < _size.x; ++p.x)
            {
                at(ivec2(p)) = (p.x / squareSize + p.y / squareSize) % 2u ? foreColor : backColor;
            }
        }
    }

    template <Numeric T, u32 n, bool constant>
    void ImageView<T, n, constant>::copy(const ImageView<T, n, true> & src) const requires (!constant)
    {
        const u32 copyWidth{min(_size.x, src._size.x)};
        const Pixel * srcR{src.row(0)};
        Pixel * dstR{row(0)};
        for (u32 y{0u}; y < _size.y; ++y, srcR -= src._image->_size.x, dstR -= _image->_size.x)
        {
            std::copy_n(srcR, copyWidth, dstR);
        }
    }

    template <Numeric T, u32 n, bool constant>
    void ImageView<T, n, constant>::copy(const Image & src) const requires (!constant)
    {
        copy(src.view());
    }

    template <Numeric T, u32 n>
    Result<Image<T, n>> read(const std::filesystem::path & file, const bool allowComponentPadding)
    {
        const Result<List<u8>> fileData{utils::readFile(file)};

        FAIL_IF(!fileData);

        s32 width, height, channels;
        u8 * const data{stbi_load_from_memory(fileData->data(), s32(fileData->size()), &width, &height, &channels, allowComponentPadding ? s32(n) : 0)};
        ScopeGuard memGuard{[data]() { STBI_FREE(data); }};

        FAIL_IF(!data);
        FAIL_IF(width <= 0 || height <= 0);
        FAIL_IF(u32(channels) > n || (!allowComponentPadding && u32(channels) < n));

        memGuard.release();
        return Image<T, n>{uivec2{u32(width), u32(height)}, std::bit_cast<Pixel<T, n> *>(data)};
    }

    Result<GrayImage> readGray(const std::filesystem::path & file)
    {
        return read<u8, 1u>(file, false);
    }

    Result<GrayAlphaImage> readGrayAlpha(const std::filesystem::path & file, const bool allowComponentPadding)
    {
        return read<u8, 2u>(file, allowComponentPadding);
    }

    Result<RgbImage> readRgb(const std::filesystem::path & file, const bool allowComponentPadding)
    {
        return read<u8, 3u>(file, allowComponentPadding);
    }

    Result<RgbaImage> readRgba(const std::filesystem::path & file, const bool allowComponentPadding)
    {
        return read<u8, 4u>(file, allowComponentPadding);
    }

    template <Numeric T, u32 n>
    bool write(const Image<T, n> & image, const std::filesystem::path & file)
    {
        const std::filesystem::path extension{file.extension()};
        if (extension == ".png")
        {
            s32 dataLength{};
            u8 * const data{stbi_write_png_to_mem(std::bit_cast<const u8 *>(image.pixels()), s32(image.width() * sizeof(Pixel<T, n>)), s32(image.width()), s32(image.height()), s32(n), &dataLength)};
            const qc::ScopeGuard dataGuard{[&]() { STBI_FREE(data); }};

            FAIL_IF(dataLength <= 0 || !data);

            FAIL_IF(!utils::writeFile(file, data, u64(dataLength)));

            return true;
        }
        else
        {
            return false; // Currently unsupported
        }
    }

    // Explicit template specialization

    template class Image<u8, 1u>;
    template class Image<u8, 2u>;
    template class Image<u8, 3u>;
    template class Image<u8, 4u>;

    template class ImageView<u8, 1u, false>;
    template class ImageView<u8, 1u, true>;
    template class ImageView<u8, 2u, false>;
    template class ImageView<u8, 2u, true>;
    template class ImageView<u8, 3u, false>;
    template class ImageView<u8, 3u, true>;
    template class ImageView<u8, 4u, false>;
    template class ImageView<u8, 4u, true>;

    template Result<GrayImage> read<u8, 1u>(const std::filesystem::path &, bool);
    template Result<GrayAlphaImage> read<u8, 2u>(const std::filesystem::path &, bool);
    template Result<RgbImage> read<u8, 3u>(const std::filesystem::path &, bool);
    template Result<RgbaImage> read<u8, 4u>(const std::filesystem::path &, bool);

    template bool write(const GrayImage &, const std::filesystem::path &);
    template bool write(const GrayAlphaImage &, const std::filesystem::path &);
    template bool write(const RgbImage &, const std::filesystem::path &);
    template bool write(const RgbaImage &, const std::filesystem::path &);
}
