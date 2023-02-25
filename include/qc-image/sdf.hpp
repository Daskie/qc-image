#pragma once

#include <qc-core/list.hpp>
#include <qc-core/vector.hpp>

#include <qc-image/image.hpp>

namespace qc::image::sdf
{
    struct Line
    {
        dvec2 p1, p2;

        nodisc bool isValid() const;
    };

    struct Curve
    {
        dvec2 p1, p2, p3;

        nodisc bool isValid() const;
    };

    struct Segment
    {
        bool isCurve;
        union
        {
            Line line;
            Curve curve;
        };

        Segment() = default;
        Segment(const dvec2 & p1, const dvec2 & p2);
        Segment(const dvec2 & p1, const dvec2 & p2, const dvec2 & p3);

        nodisc bool isValid() const;
    };

    struct Contour
    {
        List<Segment> segments{};

        void cullDegenerates();

        void transform(const dvec2 & scale, const dvec2 & translate);

        nodisc bool isValid() const;
    };

    struct Outline
    {
        List<Contour> contours{};

        void cullDegenerates();

        void transform(const dvec2 & scale, const dvec2 & translate);

        nodisc bool isValid() const;

    };

    struct OutlineInvalidError {};

    ///
    /// ...
    /// Range is the total width of the distance gradient from 0.0 to 1.0
    /// @return generated image, or empty image if `outline.isValid()` is false
    ///
    GrayImage generate(const Outline & outline, const int size, const double range);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace qc::image::sdf
{
    inline Segment::Segment(const dvec2 & p1, const dvec2 & p2) :
        isCurve{false},
        line{p1, p2}
    {}

    inline Segment::Segment(const dvec2 & p1, const dvec2 & p2, const dvec2 & p3) :
        isCurve{true},
        curve{p1, p2, p3}
    {}
}
