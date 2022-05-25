#ifndef NATIVE_SHAPE_H
#define NATIVE_SHAPE_H

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <opencv2/opencv.hpp>
#pragma clang diagnostic ppop

#include "util/json_utils.h"

enum LayoutAnchor {
    ScreenStart,
    ScreenEnd,
    IntersectStart,
    IntersectEnd,
};
EXTENDED_JSON_TYPE_ENUM(LayoutAnchor, ScreenStart, ScreenEnd, IntersectStart, IntersectEnd);

class Anchor {
public:
    Anchor(LayoutAnchor h, LayoutAnchor v) noexcept
        : h_(h)
        , v_(v) {}

    Anchor(LayoutAnchor both) noexcept  // NOLINT(google-explicit-constructor)
        : h_(both)
        , v_(both) {}

    Anchor() noexcept
        : h_(LayoutAnchor::ScreenStart)
        , v_(LayoutAnchor::ScreenStart) {}

    [[nodiscard]] inline LayoutAnchor h() const { return h_; }
    [[nodiscard]] inline LayoutAnchor v() const { return v_; }

    inline bool operator==(const Anchor &other) const { return (h_ == other.h_) && (v_ == other.v_); }
    inline bool operator==(const LayoutAnchor &other) const { return (h_ == other) && (v_ == other); }

    EXTENDED_JSON_TYPE_NDC(Anchor, h_, v_);

private:
    LayoutAnchor h_;
    LayoutAnchor v_;
};
EXTENDED_JSON_TYPE_PRINTABLE(Anchor)

template<typename T>
class Point {
public:
    Point(T x, T y, const Anchor &anchor) noexcept
        : x_(x)
        , y_(y)
        , anchor_(anchor) {}

    Point(T x, T y) noexcept
        : x_(x)
        , y_(y)
        , anchor_(LayoutAnchor::ScreenStart) {}

    Point() noexcept = default;

    [[nodiscard]] inline T x() const { return x_; }
    [[nodiscard]] inline T y() const { return y_; }
    [[nodiscard]] inline Anchor anchor() const { return anchor_; }

    inline Point<T> &operator=(const Point<T> &other) = default;

    inline Point<T> operator+(const Point<T> &other) const {
        assert(anchor_ == other.anchor_);
        return {x_ + other.x_, y_ + other.y_, anchor_};
    }

    inline Point<T> operator-(const Point<T> &other) const {
        assert(anchor_ == other.anchor_);
        return {x_ - other.x_, y_ - other.y_, anchor_};
    }

    inline Point<T> operator*(double other) const {
        return {
            static_cast<T>(static_cast<double>(x_) * other),
            static_cast<T>(static_cast<double>(y_) * other),
            anchor_,
        };
    }

    [[nodiscard]] double distance(const Point<T> &other) const {
        return std::sqrt(std::pow(x_ - other.x_, 2) + std::pow(y_ - other.y_, 2));
    }

    EXTENDED_JSON_TYPE_NDC(Point<T>, x_, y_, anchor_);

private:
    T x_;
    T y_;
    Anchor anchor_;
};
EXTENDED_JSON_TYPE_TEMPLATE_PRINTABLE(Point)

template<typename T>
class Size {
public:
    constexpr Size(T width, T height) noexcept
        : width_(width)
        , height_(height) {}

    Size(const cv::Size_<T> &size) noexcept  // NOLINT(google-explicit-constructor)
        : width_(size.width)
        , height_(size.height) {}

    [[nodiscard]] inline T width() const { return width_; }
    [[nodiscard]] inline T height() const { return height_; }

    [[nodiscard]] inline cv::Size_<T> toCVSize() const { return {width_, height_}; }

    inline Size<T> &operator=(const Size<T> &other) = default;
    inline bool operator!=(const Size<T> &other) const {
        return (width_ != other.width_) || (height_ != other.height_);
    }

    inline Size<T> operator-(const Size<T> &other) const { return {width_ - other.width_, height_ - other.height_}; }
    inline Size<T> operator/(const T &other) const { return {width_ / other, height_ / other}; }

    EXTENDED_JSON_TYPE_NDC(Size<T>, width_, height_);

private:
    T width_;
    T height_;
};
EXTENDED_JSON_TYPE_TEMPLATE_PRINTABLE(Size)

template<typename T>
class Line {
public:
    Line(const Point<T> &p1, const Point<T> &p2) noexcept
        : p1_(p1)
        , p2_(p2) {}

    [[nodiscard]] inline Point<T> p1() const { return p1_; }
    [[nodiscard]] inline Point<T> p2() const { return p2_; }

    inline Line<T> &operator=(const Line<T> &other) = default;

    [[nodiscard]] inline Point<T> pointAt(double ratio) const { return (p2_ - p1_) * ratio + p1_; }

    [[nodiscard]] inline double length() const { return p1_.distance(p2_); }

    [[nodiscard]] inline Line<T> reversed() const { return {p2_, p1_}; }

    EXTENDED_JSON_TYPE_NDC(Line<T>, p1_, p2_);

private:
    Point<T> p1_;
    Point<T> p2_;
};
EXTENDED_JSON_TYPE_TEMPLATE_PRINTABLE(Line)

template<typename T>
class Rect {
public:
    Rect(const Point<T> &top_left, const Point<T> &bottom_right) noexcept
        : top_left_(top_left)
        , bottom_right_(bottom_right) {}

    inline Rect &operator=(const Rect &other) = default;

    [[nodiscard]] inline T top_left() const { return top_left_; }
    [[nodiscard]] inline T bottom_right() const { return bottom_right_; }

    [[nodiscard]] inline T left() const { return top_left_.x(); }
    [[nodiscard]] inline T top() const { return top_left_.y(); }
    [[nodiscard]] inline T right() const { return bottom_right_.x(); }
    [[nodiscard]] inline T bottom() const { return bottom_right_.y(); }

    [[nodiscard]] inline T width() const { return bottom_right_.x() - top_left_.x(); }
    [[nodiscard]] inline T height() const { return bottom_right_.y() - top_left_.y(); }

    [[nodiscard]] inline Size<T> size() const { return {width(), height()}; }

    EXTENDED_JSON_TYPE_NDC(Rect, top_left_, bottom_right_);

private:
    Point<T> top_left_;
    Point<T> bottom_right_;
};
EXTENDED_JSON_TYPE_TEMPLATE_PRINTABLE(Rect)

#endif  //NATIVE_SHAPE_H