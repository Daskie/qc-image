#pragma once

#include <qc-core/list.hpp>
#include <qc-core/vector.hpp>

#include <qc-image/image.hpp>

namespace qc::image::sdf
{
    struct Line
    {
        dvec2 p1, p2;

        bool isValid() const noexcept;
    };

    struct Curve
    {
        dvec2 p1, p2, p3;

        bool isValid() const noexcept;
    };

    struct Segment
    {
        bool isCurve;
        union
        {
            Line line;
            Curve curve;
        };

        Segment() noexcept = default;
        Segment(const dvec2 & p1, const dvec2 & p2) noexcept;
        Segment(const dvec2 & p1, const dvec2 & p2, const dvec2 & p3) noexcept;

        bool isValid() const noexcept;
    };

    struct Contour
    {
        List<Segment> segments{};

        void cullDegenerates();

        void transform(const dvec2 & scale, const dvec2 & translate);

        bool isValid() const noexcept;
    };

    struct Outline
    {
        List<Contour> contours{};

        void cullDegenerates();

        void transform(const dvec2 & scale, const dvec2 & translate);

        bool isValid() const noexcept;
    };

    struct OutlineInvalidError {};

    ///
    /// ...
    /// Range is the total width of the distance gradient from 0.0 to 1.0
    /// @throws OutlineInvalidError if `outline.isValid()` is false
    ///
    GrayImage generate(const Outline & outline, const int size, const double range);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace qc::image::sdf
{
    inline Segment::Segment(const dvec2 & p1, const dvec2 & p2) noexcept :
        isCurve{false},
        line{p1, p2}
    {}

    inline Segment::Segment(const dvec2 & p1, const dvec2 & p2, const dvec2 & p3) noexcept :
        isCurve{true},
        curve{p1, p2, p3}
    {}
}
