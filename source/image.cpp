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
    template <typename P>
    void Image<P>::fill(const P & color)
    {
        std::fill_n(_pixels, _size.x * _size.y, color);
    }

    template <typename P>
    P * Image<P>::release()
    {
        _size = {};
        return std::exchange(_pixels, nullptr);
    }

    template <typename P>
    void ImageView<P>::fill(const P & color) const requires (!std::is_const_v<P>)
    {
        for (u32 y{0u}; y < _size.y; ++y)
        {
            std::fill_n(row(s32(y)), _size.x, color);
        }
    }

    template <typename P>
    void ImageView<P>::outline(const u32 thickness, const P & color) const requires (!std::is_const_v<P>)
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

            if (min(_size) > 2u)
            {
                view(ivec2{1}, _size - 2u).outline(thickness - 1u, color);
            }
        }
    }

    template <typename P>
    void ImageView<P>::horizontalLine(const ivec2 pos, const u32 length, const P & color) const requires (!std::is_const_v<P>)
    {
        if (pos.y >= 0 && u32(pos.y) < _size.y)
        {
            const ispan1 span{ispan1{pos.x, pos.x + s32(length)} & ispan1{_pos.x, _pos.x + s32(_size.x)}};
            P * const r{row(pos.y)};
            std::fill(r + span.min, r + span.max, color);
        }
    }

    template <typename P>
    void ImageView<P>::verticalLine(const ivec2 pos, const u32 length, const P & color) const requires (!std::is_const_v<P>)
    {
        if (pos.x >= 0 && u32(pos.x) < _size.x)
        {
            const ispan1 span{ispan1{pos.y, pos.y + s32(length)} & ispan1{_pos.y, _pos.y + s32(_size.y)}};
            P * p{&at(pos.x, span.min)};
            for (s32 y{span.min}; y < span.max; ++y, p -= _image->_size.x)
            {
                *p = color;
            }
        }
    }

    template <typename P>
    void ImageView<P>::checkerboard(const u32 squareSize, const P & backColor, const P & foreColor) const requires (!std::is_const_v<P>)
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

    template <typename P>
    void ImageView<P>::copy(const ImageView<const P> & src) const requires (!std::is_const_v<P>)
    {
        const u32 copyWidth{min(_size.x, src._size.x)};
        const P * srcR{src.row(0)};
        P * dstR{row(0)};
        for (u32 y{0u}; y < _size.y; ++y, srcR -= src._image->_size.x, dstR -= _image->_size.x)
        {
            std::copy_n(srcR, copyWidth, dstR);
        }
    }

    template <typename P>
    void ImageView<P>::copy(const _Image & src) const requires (!std::is_const_v<P>)
    {
        copy(src.view());
    }

    template <typename P>
    Result<Image<P>> read(const std::filesystem::path & file, const bool allowComponentPadding)
    {
        const Result<List<u8>> fileData{utils::readFile(file)};

        FAIL_IF(!fileData);

        int width, height, channels;
        u8 * const data{stbi_load_from_memory(fileData->data(), int(fileData->size()), &width, &height, &channels, allowComponentPadding ? int(Image<P>::components) : 0)};
        ScopeGuard memGuard{[data]() { STBI_FREE(data); }};

        FAIL_IF(!data);
        FAIL_IF(width <= 0 || height <= 0);
        FAIL_IF(u32(channels) > Image<P>::components || (!allowComponentPadding && u32(channels) < Image<P>::components));

        memGuard.release();
        return Image<P>{uivec2{u32(width), u32(height)}, std::bit_cast<P *>(data)};
    }

    Result<GrayImage> readGrayImage(const std::filesystem::path & file)
    {
        return read<u8>(file, false);
    }

    Result<GrayAlphaImage> readGrayAlphaImage(const std::filesystem::path & file, const bool allowComponentPadding)
    {
        return read<ucvec2>(file, allowComponentPadding);
    }

    Result<RgbImage> readRgbImage(const std::filesystem::path & file, const bool allowComponentPadding)
    {
        return read<ucvec3>(file, allowComponentPadding);
    }

    Result<RgbaImage> readRgbaImage(const std::filesystem::path & file, const bool allowComponentPadding)
    {
        return read<ucvec4>(file, allowComponentPadding);
    }

    template <typename P>
    bool write(const Image<P> & image, const std::filesystem::path & file)
    {
        const std::filesystem::path extension{file.extension()};
        if (extension == ".png")
        {
            int dataLength{};
            u8 * const data{stbi_write_png_to_mem(std::bit_cast<const u8*>(image.pixels()), int(image.width() * sizeof(P)), int(image.width()), int(image.height()), int(image.components), &dataLength)};
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

    template class Image<u8>;
    template class Image<ucvec2>;
    template class Image<ucvec3>;
    template class Image<ucvec4>;

    template class ImageView<u8>;
    template class ImageView<ucvec2>;
    template class ImageView<ucvec3>;
    template class ImageView<ucvec4>;

    template Result<GrayImage> read<u8>(const std::filesystem::path &, bool);
    template Result<GrayAlphaImage> read<ucvec2>(const std::filesystem::path &, bool);
    template Result<RgbImage> read<ucvec3>(const std::filesystem::path &, bool);
    template Result<RgbaImage> read<ucvec4>(const std::filesystem::path &, bool);

    template bool write(const GrayImage &, const std::filesystem::path &);
    template bool write(const GrayAlphaImage &, const std::filesystem::path &);
    template bool write(const RgbImage &, const std::filesystem::path &);
    template bool write(const RgbaImage &, const std::filesystem::path &);
}
