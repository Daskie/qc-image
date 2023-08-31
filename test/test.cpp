#include <qc-image/image.hpp>

int main()
{
    // RGB
    {
        const qc::Result<qci::RgbImage> rgbImage{qci::readRgb("rgb-in.png", false)};
        ABORT_IF(!rgbImage);
        ABORT_IF(!qci::write(*rgbImage, "rgb-out.png"));
    }
    // RGBA
    {
        const qc::Result<qci::RgbaImage> rgbaImage{qci::readRgba("rgba-in.png", false)};
        ABORT_IF(!rgbaImage);
        ABORT_IF(!qci::write(*rgbaImage, "rgba-out.png"));
    }
    // Gray
    {
        const qc::Result<qci::GrayImage> grayImage{qci::readGray("g-in.png")};
        ABORT_IF(!grayImage);
        ABORT_IF(!qci::write(*grayImage, "g-out.png"));
    }
    // GrayAlpha
    {
        const qc::Result<qci::GrayAlphaImage> grayAlphaImage{qci::readGrayAlpha("ga-in.png", false)};
        ABORT_IF(!grayAlphaImage);
        ABORT_IF(!qci::write(*grayAlphaImage, "ga-out.png"));
    }

    return 0;
}
