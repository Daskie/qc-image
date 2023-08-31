#include <qc-image/image.hpp>

int main()
{
    // RGB
    {
        const qc::Result<qc::image::RgbImage> rgbImage{qc::image::readRgb("rgb-in.png", false)};
        ABORT_IF(!rgbImage);
        ABORT_IF(!qc::image::write(*rgbImage, "rgb-out.png"));
    }
    // RGBA
    {
        const qc::Result<qc::image::RgbaImage> rgbaImage{qc::image::readRgba("rgba-in.png", false)};
        ABORT_IF(!rgbaImage);
        ABORT_IF(!qc::image::write(*rgbaImage, "rgba-out.png"));
    }
    // Gray
    {
        const qc::Result<qc::image::GrayImage> grayImage{qc::image::readGray("g-in.png")};
        ABORT_IF(!grayImage);
        ABORT_IF(!qc::image::write(*grayImage, "g-out.png"));
    }
    // GrayAlpha
    {
        const qc::Result<qc::image::GrayAlphaImage> grayAlphaImage{qc::image::readGrayAlpha("ga-in.png", false)};
        ABORT_IF(!grayAlphaImage);
        ABORT_IF(!qc::image::write(*grayAlphaImage, "ga-out.png"));
    }

    return 0;
}
