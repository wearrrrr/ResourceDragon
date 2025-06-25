#pragma once
#include "imgui.h"
#include <algorithm>
#include <cmath>

inline ImVec2 operator+(const ImVec2 &a, const ImVec2 &b) {
    return ImVec2(a.x + b.x, a.y + b.y);
}
inline ImVec2 operator-(const ImVec2 &a, const ImVec2 &b) {
    return ImVec2(a.x - b.x, a.y - b.y);
}
inline ImVec2 operator*(const ImVec2 &a, const ImVec2 &b) {
    return ImVec2(a.x * b.x, a.y * b.y);
}
inline ImVec2 operator*(const ImVec2 &a, float b) {
    return ImVec2(a.x * b, a.y * b);
}
inline ImVec2 operator/(const ImVec2 &a, const ImVec2 &b) {
    return ImVec2(a.x / b.x, a.y / b.y);
}
inline ImVec2 operator/(const ImVec2 &a, float b) {
    return ImVec2(a.x / b, a.y / b);
}

// Compound assignment
inline ImVec2& operator+=(ImVec2 &a, const ImVec2 &b) {
    a.x += b.x; a.y += b.y; return a;
}
inline ImVec2& operator-=(ImVec2 &a, const ImVec2 &b) {
    a.x -= b.x; a.y -= b.y; return a;
}
inline ImVec2& operator*=(ImVec2 &a, const ImVec2 &b) {
    a.x *= b.x; a.y *= b.y; return a;
}
inline ImVec2& operator*=(ImVec2 &a, float b) {
    a.x *= b; a.y *= b; return a;
}
inline ImVec2& operator/=(ImVec2 &a, const ImVec2 &b) {
    a.x /= b.x; a.y /= b.y; return a;
}
inline ImVec2& operator/=(ImVec2 &a, float b) {
    a.x /= b; a.y /= b; return a;
}

// Utility functions
inline ImVec2 Clamp(const ImVec2 &a, const ImVec2 &min, const ImVec2 &max) {
    return ImVec2(std::clamp(a.x, min.x, max.x), std::clamp(a.y, min.y, max.y));
}
inline ImVec2 Floor(const ImVec2 &a) {
    return ImVec2(floorf(a.x), floorf(a.y));
}
inline ImVec2 Ceil(const ImVec2 &a) {
    return ImVec2(ceilf(a.x), ceilf(a.y));
}
inline ImVec2 Round(const ImVec2 &a) {
    return ImVec2(roundf(a.x), roundf(a.y));
}
