#include <qc-image/sdf.hpp>

#include <qc-core/math.hpp>

namespace qci::sdf
{
    namespace
    {
        struct _LineExt
        {
            fvec2 a;
            f32 invLength2;
        };

        struct _CurveExt
        {
            fvec2 a;
            fvec2 b;
            fvec2 c;
            f32 maxHalfSubLineLength;
        };

        union _SegmentExt
        {
            _LineExt line;
            _CurveExt curve;
        };

        struct _Row
        {
            f32 * distances;
            u32 interceptN;
            f32 * intercepts;
        };

        bool _isPointValid(const fvec2 p)
        {
            // Must not be NaN or too big
            return abs(p.x) <= 1.0e9f && abs(p.y) <= 1.0e9f;
        }

        fvec2 _evaluateBezier(const _CurveExt & curve, const f32 t)
        {
            return curve.a * t * t + curve.b * t + curve.c;
        }

        fvec2 _evaluateBezier(const _CurveExt & curve, const fvec2 t)
        {
            return curve.a * t * t + curve.b * t + curve.c;
        }

        fspan2 _detSpan(const Line & line)
        {
            return {min(line.p1, line.p2), max(line.p1, line.p2)};
        }

        fspan2 _detSpan(const Curve & curve, const _CurveExt & curveExt)
        {
            fspan2 span{min(curve.p1, curve.p3), max(curve.p1, curve.p3)};

            if (!span.contains(curve.p2))
            {
                const fvec2 extremeT{clamp(curveExt.b / (-2.0f * curveExt.a), 0.0f, 1.0f)};
                const fvec2 extremeP{_evaluateBezier(curveExt, extremeT)};
                minify(span.min, extremeP);
                maxify(span.max, extremeP);
            }

            return span;
        }

        fspan2 _detSpan(const Segment & segment, const _SegmentExt & segmentExt)
        {
            return segment.isCurve ? _detSpan(segment.curve, segmentExt.curve) : _detSpan(segment.line);
        }

        f32 _distance2To(const Line & line, const _LineExt & lineExt, const fvec2 p)
        {
            const fvec2 b{p - line.p1};
            const f32 t{clamp(dot(lineExt.a, b) * lineExt.invLength2, 0.0f, 1.0f)};
            const fvec2 c{t * lineExt.a};
            return distance2(b, c);
        }

        f32 _findClosestPoint(const _CurveExt & curve, const fvec2 p, f32 lowT, f32 highT)
        {
            f32 midT{(lowT + highT) * 0.5f};
            fvec2 lowB{_evaluateBezier(curve, lowT)};
            fvec2 midB{_evaluateBezier(curve, midT)};
            fvec2 highB{_evaluateBezier(curve, highT)};
            f32 lowDist2{distance2(p, lowB)};
            f32 midDist2{distance2(p, midB)};
            f32 highDist2{distance2(p, highB)};
            f32 minDist2{min(min(lowDist2, midDist2), highDist2)};
            f32 halfLength{(highT - lowT) * 0.5f};

            while (halfLength > curve.maxHalfSubLineLength)
            {
                halfLength *= 0.5f;

                const f32 t1{midT - halfLength};
                const f32 t2{midT + halfLength};
                const fvec2 b1{_evaluateBezier(curve, t1)};
                const fvec2 b2{_evaluateBezier(curve, t2)};
                const f32 d1{distance2(p, b1)};
                const f32 d2{distance2(p, b2)};

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

        f32 _distance2To(const Curve &, const _CurveExt & curveExt, const fvec2 p)
        {
            // Point of maximum curvature
            const f32 d{-2.0f * magnitude2(curveExt.a)};
            const f32 u{d == 0.0f ? 0.0f : clamp(dot(curveExt.a, curveExt.b) / d, 0.0f, 1.0f)};

            f32 dist2{number::inf<f32>};

            if (u > 0.0f)
            {
                minify(dist2, _findClosestPoint(curveExt, p, 0.0f, u));
            }

            if (u < 1.0f)
            {
                minify(dist2, _findClosestPoint(curveExt, p, u, 1.0f));
            }

            return dist2;
        }

        f32 _distance2To(const Segment & segment, const _SegmentExt & segmentExt, const fvec2 p)
        {
            return segment.isCurve ? _distance2To(segment.curve, segmentExt.curve, p) : _distance2To(segment.line, segmentExt.line, p);
        }

        void _updateDistances(const Segment & segment, const _SegmentExt & segmentExt, const u32 size, const f32 halfRange, _Row * const rows, const fspan2 bounds)
        {
            const ispan2 pixelBounds{max(floor<s32>(bounds.min - halfRange), 0), min(ceil<s32>(bounds.max + halfRange), s32(size))};

            for (ivec2 p{pixelBounds.min}; p.y < pixelBounds.max.y; ++p.y)
            {
                _Row & row{rows[p.y]};

                for (p.x = pixelBounds.min.x; p.x < pixelBounds.max.x; ++p.x)
                {
                    minify(row.distances[p.x], _distance2To(segment, segmentExt, fvec2(p) + 0.5f));
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

            const fvec2 delta{line.p2 - line.p1};
            const f32 slope{delta.x / delta.y};
            const f32 offset{line.p1.x - slope * line.p1.y};

            for (s32 yPx{interceptRows.min}; yPx <= interceptRows.max; ++yPx)
            {
                _Row & row{rows[yPx]};

                fvec2 intercept;
                intercept.y = f32(yPx) + 0.5f;
                intercept.x = slope * intercept.y + offset;

                // Explicitly disallow endpoint intercepts
                if (intercept != line.p1 && intercept != line.p2)
                {
                    row.intercepts[row.interceptN++] = intercept.x;
                }
            }
        }

        void _updateIntercepts(const Curve & curve, const _CurveExt & curveExt, _Row * const rows, const ispan1 & interceptRows)
        {
            for (s32 yPx{interceptRows.min}; yPx <= interceptRows.max; ++yPx)
            {
                _Row & row{rows[yPx]};

                const f32 y{f32(yPx) + 0.5f};

                const Duo<f32> roots{quadraticRoots(curveExt.a.y, curveExt.b.y, curveExt.c.y - y)};

                for (const f32 t : roots)
                {
                    if (t > 0.0f && t < 1.0f)
                    {
                        const fvec2 intercept{_evaluateBezier(curveExt, t)};

                        // Explicitly disallow endpoint intercepts
                        if (intercept != curve.p1 && intercept != curve.p2)
                        {
                            row.intercepts[row.interceptN++] = intercept.x;
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
                .invLength2 = 1.0f / distance2(line.p1, line.p2)};
        }

        _CurveExt _calcExtra(const Curve & curve)
        {
            return _CurveExt{
                .a = curve.p1 - 2.0f * curve.p2 + curve.p3,
                .b = 2.0f * (curve.p2 - curve.p1),
                .c = curve.p1,
                .maxHalfSubLineLength = 1.0f / (distance(curve.p1, curve.p2) + distance(curve.p2, curve.p3))};
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

        void _process(const Segment & segment, const u32 size, const f32 halfRange, _Row * const rows)
        {
            const _SegmentExt segmentExt{_calcExtra(segment)};

            const fspan2 bounds{_detSpan(segment, segmentExt)};

            _updateDistances(segment, segmentExt, size, halfRange, rows, bounds);

            ispan1 interceptRows{ceil<s32>(bounds.min.y - 0.5f), floor<s32>(bounds.max.y - 0.5f)};
            if (f32(interceptRows.min) + 0.5f == bounds.min.y) ++interceptRows.min;
            if (f32(interceptRows.max) + 0.5f == bounds.max.y) --interceptRows.max;
            clampify(interceptRows, 0, s32(size) - 1);
            if (interceptRows.max >= interceptRows.min)
            {
                _updateIntercepts(segment, segmentExt, rows, interceptRows);
            }
        }

        void _updatePointIntercepts(const Contour & contour, _Row * const rows, const u32 size)
        {
            struct Point { fvec2 p; f32 prevY, nextY; };
            static thread_local List<Point> points;

            points.resize(contour.segments.size());

            for (u32 i{0u}, endI{contour.segments.size()}; i < endI; ++i)
            {
                u32 nextI{i + 1u};
                if (nextI == endI) nextI = 0u;

                const Segment & segment{contour.segments[i]};
                Point & point{points[i]};
                Point & nextPoint{points[nextI]};

                if (segment.isCurve)
                {
                    point.p = segment.curve.p1;
                    point.nextY = segment.curve.p2.y == segment.curve.p1.y ? segment.curve.p3.y : segment.curve.p2.y;
                    nextPoint.prevY = segment.curve.p2.y == segment.curve.p3.y ? segment.curve.p1.y : segment.curve.p2.y;
                }
                else
                {
                    point.p = segment.line.p1;
                    point.nextY = segment.line.p2.y;
                    nextPoint.prevY = segment.line.p1.y;
                }
            }

            // Remove consecutive points on same y value
            for (u32 i{0u}; i < points.size(); ++i)
            {
                Point & point{points[i]};
                if (point.p.y == point.prevY)
                {
                    Point & prevPoint{points[(i + points.size() - 1u) % points.size()]};
                    Point & nextPoint{points[(i + 1u) % points.size()]};
                    prevPoint.nextY = point.nextY;
                    nextPoint.prevY = point.prevY;
                    points.erase(points.begin() + i);
                    --i;
                }
            }

            for (const Point & point : points)
            {
                if (point.p.y > 0.0f)
                {
                    const auto [f, i]{fract_i<s32>(point.p.y)};
                    if (f == 0.5f && i < s32(size))
                    {
                        // Only an intersection if the adjacent points are on opposite sides of the scanline
                        if ((point.prevY < point.p.y && point.nextY > point.p.y) || (point.prevY > point.p.y && point.nextY < point.p.y))
                        {
                            _Row & row{rows[i]};
                            row.intercepts[row.interceptN++] = point.p.x;
                        }
                    }
                }
            }
        }
    }

    bool Line::isValid() const
    {
        return _isPointValid(p1) && _isPointValid(p2) && p1 != p2;
    }

    bool Curve::isValid() const
    {
        return _isPointValid(p1) && _isPointValid(p2) && _isPointValid(p3) && p1 != p2 && p2 != p3 && p3 != p1;
    }

    bool Segment::isValid() const
    {
        return isCurve ? curve.isValid() : line.isValid();
    }

    bool Contour::isValid() const
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
        for (u32 i{1u}; i < segments.size(); ++i)
        {
            const Segment & segment1{segments[i - 1u]};
            const Segment & segment2{segments[i]};
            const fvec2 p1{segment1.isCurve ? segment1.curve.p3 : segment1.line.p2};
            const fvec2 p2{segment2.isCurve ? segment2.curve.p1 : segment2.line.p1};
            if (p1 != p2)
            {
                return false;
            }
        }

        // Last segment must connect to the first
        {
            const Segment & segment1{segments.back()};
            const Segment & segment2{segments.front()};
            const fvec2 p1{segment1.isCurve ? segment1.curve.p3 : segment1.line.p2};
            const fvec2 p2{segment2.isCurve ? segment2.curve.p1 : segment2.line.p1};
            if (p1 != p2)
            {
                return false;
            }
        }

        return true;
    }

    void Contour::normalize()
    {
        segments.eraseIf(
            [](Segment & segment)
            {
                if (segment.isCurve)
                {
                    if (zeroish(cross(segment.curve.p1 - segment.curve.p2, segment.curve.p3 - segment.curve.p2)))
                    {
                        // Convert curve to line
                        segment.isCurve = false;
                        segment.line = Line{.p1 = segment.curve.p1, .p2 = segment.curve.p3};
                    }
                    else
                    {
                        return false;
                    }
                }

                return segment.line.p1 == segment.line.p2;
            });
    }

    void Contour::transform(const fvec2 scale, const fvec2 translate)
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

    void Outline::normalize()
    {
        contours.eraseIf(
            [](Contour & contour)
            {
                contour.normalize();
                return !contour.segments;
            });
    }

    void Outline::transform(const fvec2 scale, const fvec2 translate)
    {
        for (Contour & contour : contours)
        {
            contour.transform(scale, translate);
        }
    }

    bool Outline::isValid() const
    {
        if (!contours)
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

    GrayImage generate(const Outline & outline, const u32 size, const f32 range)
    {
        static thread_local List<f32> distances{};
        static thread_local List<f32> rowIntercepts{};
        static thread_local List<_Row> rows{};

        FAIL_IF(!outline.isValid());

        // Count total segments
        u32 segmentN{0u};
        for (const Contour & contour : outline.contours)
        {
            segmentN += contour.segments.size();
        }

        // Reset buffers
        {
            distances.resize(size * size);
            for (f32 & distance : distances) distance = number::inf<float>;

            const u32 maxInterceptN{segmentN * 2u};
            rowIntercepts.resize(size * maxInterceptN);

            rows.resize(size);
            f32 * firstDistance{distances.data() + size * size - size};
            f32 * firstIntercept{rowIntercepts.data()};
            for (_Row & row : rows)
            {
                row.distances = firstDistance;
                row.interceptN = 0u;
                row.intercepts = firstIntercept;

                firstDistance -= size;
                firstIntercept += maxInterceptN;
            }
        }

        // Process segments to calculate distances and row intersections

        const f32 halfRange{range * 0.5f};

        for (const Contour & contour : outline.contours)
        {
            for (const Segment & segment : contour.segments)
            {
                _process(segment, size, halfRange, rows.data());
            }

            // Explicitly and carefully add endpoints as intercepts if appropriate
            _updatePointIntercepts(contour, rows.data(), size);
        }

        // Sqrt distances

        for (f32 & distance : distances)
        {
            distance = std::sqrt(distance);
        }

        // Sort row intersections and invert internal distances

        for (_Row & row : rows)
        {
            std::sort(row.intercepts, row.intercepts + row.interceptN);

            // There should always be an even number of intercepts
            if (row.interceptN % 2u)
            {
                if constexpr (debug)
                {
                    ABORT();
                }
                else
                {
                    --row.interceptN;
                }
            }

            for (u32 i{1u}; i < row.interceptN; i += 2u)
            {
                const fspan1 xSpan{row.intercepts[i - 1u], row.intercepts[i]};
                ispan1 xSpanPx{ceil<s32>(xSpan.min - 0.5f), floor<s32>(xSpan.max - 0.5f)};
                clampify(xSpanPx, 0, s32(size) - 1);
                for (s32 xPx{xSpanPx.min}; xPx <= xSpanPx.max; ++xPx)
                {
                    f32 & distance{row.distances[xPx]};
                    distance = -distance;
                }
            }
        }

        // Convert to grayscale image

        GrayImage image{size, size};

        const f32 * src{distances.data()};
        const f32 * const srcEnd{src + size * size};
        u8 * dst{image.pixels()};
        const f32 invRange{1.0f / range};

        for (; src < srcEnd; ++src, ++dst)
        {
            *dst = transnorm<u8>(0.5f - *src * invRange);
        }

        return image;
    }
}
