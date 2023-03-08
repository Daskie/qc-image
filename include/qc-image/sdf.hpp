#pragma once

#include <qc-core/list.hpp>
#include <qc-core/vector.hpp>

#include <qc-image/image.hpp>

namespace qc::image::sdf
{
    struct Line
    {
        fvec2 p1, p2;

        nodisc bool isValid() const;
    };

    struct Curve
    {
        fvec2 p1, p2, p3;

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
        Segment(fvec2 p1, fvec2 p2);
        Segment(fvec2 p1, fvec2 p2, fvec2 p3);

        nodisc bool isValid() const;
    };

    struct Contour
    {
        List<Segment> segments{};

        void normalize();

        void transform(fvec2 scale, fvec2 translate);

        nodisc bool isValid() const;
    };

    struct Outline
    {
        List<Contour> contours{};

        void normalize();

        void transform(fvec2 scale, fvec2 translate);

        nodisc bool isValid() const;

    };

    struct OutlineInvalidError {};

    ///
    /// ...
    /// Range is the total width of the distance gradient from 0.0 to 1.0
    /// @return generated image, or empty image if `outline.isValid()` is false
    ///
    GrayImage generate(const Outline & outline, const int size, const float range);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace qc::image::sdf
{
    inline Segment::Segment(fvec2 p1, fvec2 p2) :
        isCurve{false},
        line{p1, p2}
    {}

    inline Segment::Segment(fvec2 p1, fvec2 p2, fvec2 p3) :
        isCurve{true},
        curve{p1, p2, p3}
    {}
}
