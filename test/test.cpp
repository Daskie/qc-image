#include <qc-image/image.hpp>

int main()
{
    // RGB
    {
        qc::image::RgbImage rgbImage{qc::image::read<qc::ucvec3>("rgb-in.png")};
        qc::image::write(rgbImage, "rgb-out.png");
    }
    // RGBA
    {
        qc::image::RgbaImage rgbaImage{qc::image::read<qc::ucvec4>("rgba-in.png")};
        qc::image::write(rgbaImage, "rgba-out.png");
    }
    // Gray
    {
        qc::image::GrayImage gImage{qc::image::read<qc::u8>("g-in.png")};
        qc::image::write(gImage, "g-out.png");
    }
    // GrayAlpha
    {
        qc::image::GrayAlphaImage gaImage{qc::image::read<qc::ucvec2>("ga-in.png")};
        qc::image::write(gaImage, "ga-out.png");
    }

    return 0;
}
