#include <qc-image/sdf.hpp>

#include <qc-core/math.hpp>

namespace qc::image::sdf
{
    namespace
    {
        struct _LineExt
        {
            dvec2 a;
            double invLength2;
        };

        struct _CurveExt
        {
            dvec2 a;
            dvec2 b;
            dvec2 c;
            double maxHalfSubLineLength;
        };

        union _SegmentExt
        {
            _LineExt line;
            _CurveExt curve;
        };

        struct _Row
        {
            double * distances;
            int interceptCount;
            double * intercepts;
        };

        bool _isPointValid(const dvec2 & p)
        {
            // Must not be NaN or too big
            return abs(p.x) <= 1.0e9 && abs(p.y) <= 1.0e9;
        }

        dvec2 _evaluateBezier(const _CurveExt & curve, const double t)
        {
            return curve.a * t * t + curve.b * t + curve.c;
        }

        dvec2 _evaluateBezier(const _CurveExt & curve, const dvec2 & t)
        {
            return curve.a * t * t + curve.b * t + curve.c;
        }

        dspan2 _detSpan(const Line & line)
        {
            return {min(line.p1, line.p2), max(line.p1, line.p2)};
        }

        dspan2 _detSpan(const Curve & curve, const _CurveExt & curveExt)
        {
            dspan2 span{min(curve.p1, curve.p3), max(curve.p1, curve.p3)};

            if (!span.contains(curve.p2))
            {
                const dvec2 extremeT{clamp(curveExt.b / (-2.0 * curveExt.a), 0.0, 1.0)};
                const dvec2 extremeP{_evaluateBezier(curveExt, extremeT)};
                minify(span.min, extremeP);
                maxify(span.max, extremeP);
            }

            return span;
        }

        dspan2 _detSpan(const Segment & segment, const _SegmentExt & segmentExt)
        {
            return segment.isCurve ? _detSpan(segment.curve, segmentExt.curve) : _detSpan(segment.line);
        }

        double _distance2To(const Line & line, const _LineExt & lineExt, const dvec2 & p)
        {
            const dvec2 b{p - line.p1};
            const double t{clamp(dot(lineExt.a, b) * lineExt.invLength2, 0.0, 1.0)};
            const dvec2 c{t * lineExt.a};
            return distance2(b, c);
        }

        double _findClosestPoint(const _CurveExt & curve, const dvec2 & p, double lowT, double highT)
        {
            double midT{(lowT + highT) * 0.5};
            dvec2 lowB{_evaluateBezier(curve, lowT)};
            dvec2 midB{_evaluateBezier(curve, midT)};
            dvec2 highB{_evaluateBezier(curve, highT)};
            double lowDist2{distance2(p, lowB)};
            double midDist2{distance2(p, midB)};
            double highDist2{distance2(p, highB)};
            double minDist2{min(min(lowDist2, midDist2), highDist2)};
            double halfLength{(highT - lowT) * 0.5};

            while (halfLength > curve.maxHalfSubLineLength)
            {
                halfLength *= 0.5;

                const double t1{midT - halfLength};
                const double t2{midT + halfLength};
                const dvec2 b1{_evaluateBezier(curve, t1)};
                const dvec2 b2{_evaluateBezier(curve, t2)};
                const double d1{distance2(p, b1)};
                const double d2{distance2(p, b2)};

                minDist2 = min(minDist2, d1, d2);

                if (min(lowDist2, d1) <= minDist2)
                {
                    highT = midT;
                    highB = midB;
                    highDist2 = midDist2;
                    midT = t1;
                    midB = b1;
                    midDist2 = d1;
                }
                else if (min(highDist2, d2) <= minDist2)
                {
                    lowT = midT;
                    lowB = midB;
                    lowDist2 = midDist2;
                    midT = t2;
                    midB = b2;
                    midDist2 = d2;
                }
                else
                {
                    lowT = t1;
                    lowB = b1;
                    lowDist2 = d1;
                    highT = t2;
                    highB = b2;
                    highDist2 = d2;
                }
            }

            return distance2ToLine(lowB, highB, p);
        }

        double _distance2To(const Curve &, const _CurveExt & curveExt, const dvec2 & p)
        {
            // Point of maximum curvature
            const double d{-2.0 * magnitude2(curveExt.a)};
            const double u{d == 0.0 ? 0.0 : clamp(dot(curveExt.a, curveExt.b) / d, 0.0, 1.0)};

            double dist2{infinity<double>};

            if (u > 0.0)
            {
                minify(dist2, _findClosestPoint(curveExt, p, 0.0, u));
            }

            if (u < 1.0)
            {
                minify(dist2, _findClosestPoint(curveExt, p, u, 1.0));
            }

            return dist2;
        }

        double _distance2To(const Segment & segment, const _SegmentExt & segmentExt, const dvec2 & p)
        {
            return segment.isCurve ? _distance2To(segment.curve, segmentExt.curve, p) : _distance2To(segment.line, segmentExt.line, p);
        }

        void _updateDistances(const Segment & segment, const _SegmentExt & segmentExt, const int size, const double range, _Row * const rows, const dspan2 bounds)
        {
            const ispan2 pixelBounds{max(floor<int>(bounds.min - range), 0), min(ceil<int>(bounds.max + range), size)};

            for (ivec2 p{pixelBounds.min}; p.y < pixelBounds.max.y; ++p.y)
            {
                _Row & row{rows[p.y]};

                for (p.x = pixelBounds.min.x; p.x < pixelBounds.max.x; ++p.x)
                {
                    minify(row.distances[p.x], _distance2To(segment, segmentExt, dvec2(p) + 0.5));
                }
            }
        }

        void _updateIntercepts(const Line & line, const _LineExt &, _Row * const rows, const ispan1 & interceptRows)
        {
            // Perfectly horizontal line has no intercepts
            if (line.p1.y == line.p2.y)
            {
                return;
            }

            const dvec2 delta{line.p2 - line.p1};
            const double slope{delta.x / delta.y};
            const double offset{line.p1.x - slope * line.p1.y};

            for (int yPx{interceptRows.min}; yPx <= interceptRows.max; ++yPx)
            {
                _Row & row{rows[yPx]};

                dvec2 intercept;
                intercept.y = double(yPx) + 0.5;
                intercept.x = slope * intercept.y + offset;

                // Explicitly disallow endpoint intercepts
                if (intercept != line.p1 && intercept != line.p2)
                {
                    row.intercepts[row.interceptCount++] = intercept.x;
                }
            }
        }

        void _updateIntercepts(const Curve & curve, const _CurveExt & curveExt, _Row * const rows, const ispan1 & interceptRows)
        {
            for (int yPx{interceptRows.min}; yPx <= interceptRows.max; ++yPx)
            {
                _Row & row{rows[yPx]};

                const double y{double(yPx) + 0.5};

                const Duo<double> roots{quadraticRoots(curveExt.a.y, curveExt.b.y, curveExt.c.y - y)};

                for (const double t : roots)
                {
                    if (t > 0.0 && t < 1.0)
                    {
                        const dvec2 intercept{_evaluateBezier(curveExt, t)};

                        // Explicitly disallow endpoint intercepts
                        if (intercept != curve.p1 && intercept != curve.p2)
                        {
                            row.intercepts[row.interceptCount++] = intercept.x;
                        }
                    }
                }
            }
        }

        void _updateIntercepts(const Segment & segment, const _SegmentExt & segmentExt, _Row * const rows, const ispan1 & interceptRows)
        {
            return segment.isCurve ?
                _updateIntercepts(segment.curve, segmentExt.curve, rows, interceptRows) :
                _updateIntercepts(segment.line, segmentExt.line, rows, interceptRows);
        }

        _LineExt _calcExtra(const Line & line)
        {
            return _LineExt{
                .a = line.p2 - line.p1,
                .invLength2 = 1.0 / distance2(line.p1, line.p2)};
        }

        _CurveExt _calcExtra(const Curve & curve)
        {
            return _CurveExt{
                .a = curve.p1 - 2.0 * curve.p2 + curve.p3,
                .b = 2.0 * (curve.p2 - curve.p1),
                .c = curve.p1,
                .maxHalfSubLineLength = 1.0 / (distance(curve.p1, curve.p2) + distance(curve.p2, curve.p3))};
        }

        _SegmentExt _calcExtra(const Segment & segment)
        {
            if (segment.isCurve)
            {
                return {.curve = _calcExtra(segment.curve)};
            }
            else
            {
                return {.line = _calcExtra(segment.line)};
            }
        }

        void _process(const Segment & segment, const int size, const double range, _Row * const rows)
        {
            const _SegmentExt segmentExt{_calcExtra(segment)};

            const dspan2 bounds{_detSpan(segment, segmentExt)};

            _updateDistances(segment, segmentExt, size, range, rows, bounds);

            ispan1 interceptRows{ceil<int>(bounds.min.y - 0.5), floor<int>(bounds.max.y - 0.5)};
            if (double(interceptRows.min) + 0.5 == bounds.min.y) ++interceptRows.min;
            if (double(interceptRows.max) + 0.5 == bounds.max.y) --interceptRows.max;
            clampify(interceptRows, 0, size - 1);
            if (interceptRows.max >= interceptRows.min)
            {
                _updateIntercepts(segment, segmentExt, rows, interceptRows);
            }
        }

        void _updatePointIntercepts(const Segment & segment1, const Segment & segment2, _Row * const rows, const int size)
        {
            const dvec2 p{segment1.isCurve ? segment1.curve.p3 : segment1.line.p2};

            if (p.y > 0.0)
            {
                const auto [f, i]{fract_i<int>(p.y)};
                if (f == 0.5 && i < size)
                {
                    const dvec2 p1{segment1.isCurve ? segment1.curve.p2 : segment1.line.p1};
                    const dvec2 p2{segment2.isCurve ? segment2.curve.p2 : segment2.line.p2};

                    // Only an intersection if the adjacent points are on opposite sides of the scanline
                    if (abs(sign(p1.y - p.y) - sign(p2.y - p.y)) == 2)
                    {
                        _Row & row{rows[i]};
                        row.intercepts[row.interceptCount++] = p.x;
                    }
                }
            }
        }

        void _updatePointIntercepts(const Contour & contour, _Row * const rows, const int size)
        {
            for (unat i{0u}, n{contour.segments.size() - 1u}; i < n; ++i)
            {
                _updatePointIntercepts(contour.segments[i], contour.segments[i + 1u], rows, size);
            }

            _updatePointIntercepts(contour.segments.back(), contour.segments.front(), rows, size);
        }
    }

    bool Line::isValid() const noexcept
    {
        return _isPointValid(p1) && _isPointValid(p2) && p1 != p2;
    }

    bool Curve::isValid() const noexcept
    {
        return _isPointValid(p1) && _isPointValid(p2) && _isPointValid(p3) && p1 != p2 && p2 != p3 && p3 != p1;
    }

    bool Segment::isValid() const noexcept
    {
        return isCurve ? curve.isValid() : line.isValid();
    }

    bool Contour::isValid() const noexcept
    {
        if (segments.size() < 2u)
        {
            return false;
        }

        // All segments must be themselves valid
        for (const Segment & segment : segments)
        {
            if (!segment.isValid())
            {
                return false;
            }
        }

        // Each segment must connect to the next
        for (unat i{1u}; i < segments.size(); ++i)
        {
            const Segment & segment1{segments[i - 1u]};
            const Segment & segment2{segments[i]};
            const dvec2 & p1{segment1.isCurve ? segment1.curve.p3 : segment1.line.p2};
            const dvec2 & p2{segment2.isCurve ? segment2.curve.p1 : segment2.line.p1};
            if (p1 != p2)
            {
                return false;
            }
        }

        // Last segment must connect to the first
        {
            const Segment & segment1{segments.back()};
            const Segment & segment2{segments.front()};
            const dvec2 & p1{segment1.isCurve ? segment1.curve.p3 : segment1.line.p2};
            const dvec2 & p2{segment2.isCurve ? segment2.curve.p1 : segment2.line.p1};
            if (p1 != p2)
            {
                return false;
            }
        }

        return true;
    }

    void Contour::cullDegenerates()
    {
        std::erase_if(
            segments,
            [](Segment & segment)
            {
                if (segment.isCurve)
                {
                    if (segment.curve.p1 == segment.curve.p3)
                    {
                        return true;
                    }

                    if (segment.curve.p1 == segment.curve.p2 || segment.curve.p3 == segment.curve.p2)
                    {
                        segment.isCurve = false;
                        segment.line = Line{.p1 = segment.curve.p1, .p2 = segment.curve.p3};
                    }

                    return false;
                }
                else
                {
                    return segment.line.p1 == segment.line.p2;
                }
            });
    }

    void Contour::transform(const dvec2 & scale, const dvec2 & translate)
    {
        for (Segment & segment : segments)
        {
            if (segment.isCurve)
            {
                segment.curve.p1 *= scale;
                segment.curve.p1 += translate;
                segment.curve.p2 *= scale;
                segment.curve.p2 += translate;
                segment.curve.p3 *= scale;
                segment.curve.p3 += translate;
            }
            else
            {
                segment.line.p1 *= scale;
                segment.line.p1 += translate;
                segment.line.p2 *= scale;
                segment.line.p2 += translate;
            }
        }
    }

    void Outline::cullDegenerates()
    {
        std::erase_if(
            contours,
            [](Contour & contour)
            {
                contour.cullDegenerates();
                return contour.segments.empty();
            });
    }

    void Outline::transform(const dvec2 & scale, const dvec2 & translate)
    {
        for (Contour & contour : contours)
        {
            contour.transform(scale, translate);
        }
    }

    bool Outline::isValid() const noexcept
    {
        if (contours.empty())
        {
            return false;
        }

        for (const Contour & contour : contours)
        {
            if (!contour.isValid())
            {
                return false;
            }
        }

        return true;
    }

    GrayImage generate(const Outline & outline, const int size, const double range)
    {
        static thread_local std::vector<double> distances{};
        static thread_local std::vector<double> rowIntercepts{};
        static thread_local std::vector<_Row> rows{};

        if (!outline.isValid())
        {
            throw OutlineInvalidError{};
        }

        // Count total segments
        unat segmentCount{0u};
        for (const Contour & contour : outline.contours)
        {
            segmentCount += contour.segments.size();
        }

        // Reset buffers
        // TODO: Use PoD vector which doesn't zero on resize
        {
            distances.resize(unat(size * size));
            for (double & distance : distances) distance = infinity<double>;

            const unat maxInterceptCount{segmentCount * 2u};
            rowIntercepts.resize(unat(size) * maxInterceptCount);

            rows.resize(unat(size));
            double * firstDistance{distances.data() + size * size - size};
            double * firstIntercept{rowIntercepts.data()};
            for (_Row & row: rows)
            {
                row.distances = firstDistance;
                row.interceptCount = 0u;
                row.intercepts = firstIntercept;

                firstDistance -= size;
                firstIntercept += maxInterceptCount;
            }
        }

        // Process segments to calculate distances and row intersections

        for (const Contour & contour : outline.contours)
        {
            for (const Segment & segment : contour.segments)
            {
                _process(segment, size, range, rows.data());
            }

            // Explicitly and carefully add endpoints as intercepts if appropriate
            _updatePointIntercepts(contour, rows.data(), size);
        }

        // Sqrt distances

        for (double & distance : distances)
        {
            distance = std::sqrt(distance);
        }

        // Sort row intersections and invert internal distances

        const double fSize{double(size)};

        for (_Row & row : rows)
        {
            std::sort(row.intercepts, row.intercepts + row.interceptCount);

            // There should always be an even number of intercepts
            if (row.interceptCount % 2)
            {
                assert(false);
                --row.interceptCount;
            }

            for (int i{1}; i < row.interceptCount; i += 2)
            {
                dspan1 xSpan{row.intercepts[i - 1], row.intercepts[i]};
                clampify(xSpan, 0.0, fSize);

                const ispan1 xSpanPx{ceil<int>(xSpan.min - 0.5), floor<int>(xSpan.max - 0.5)};

                for (int xPx{xSpanPx.min}; xPx <= xSpanPx.max; ++xPx)
                {
                    double & distance{row.distances[xPx]};
                    distance = -distance;
                }
            }
        }

        // Convert to grayscale image

        GrayImage image{size, size};

        const double * src{distances.data()};
        const double * const srcEnd{src + size * size};
        u8 * dst{image.pixels()};
        const double invRange{1.0 / range};

        for (; src < srcEnd; ++src, ++dst)
        {
            *dst = transnorm<u8>(0.5 - 0.5 * *src * invRange);
        }

        return image;
    }
}
