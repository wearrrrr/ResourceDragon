#include <type_traits>
#include <cstdlib>

#pragma once

template <typename T, typename U = T>
struct Vec2 {
    T x;
    U y;

    Vec2() {
        if constexpr (std::is_pointer_v<T>) {
            using pointee = std::remove_pointer_t<T>;
            this->x = static_cast<T>(malloc(sizeof(pointee)));
            this->y = static_cast<U>(malloc(sizeof(pointee)));
        }
    };

    Vec2(T x, U y) : x(x), y(y) {}

    Vec2& operator+(const Vec2& other) const {
        return Vec2(x + other.x, y + other.y);
    }

    Vec2& operator++() {
        x++;
        y++;
        return *this;
    }

    Vec2& operator+=(const Vec2& other) const {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vec2& operator-(const Vec2& other) const {
        return Vec2(x - other.x, y - other.y);
    }

    Vec2& operator-=(const Vec2& other) const {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vec2& operator--() {
        x--;
        y--;
        return *this;
    }

    Vec2& operator*=(const Vec2& other) const {
        x *= other.x;
        y *= other.y;
        return *this;
    }

    Vec2& operator/=(const Vec2& other) const {
        x /= other.x;
        y /= other.y;
        return *this;
    }

    Vec2& operator*=(const T& scalar) const {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    Vec2& operator/=(const T& scalar) const {
        x /= scalar;
        y /= scalar;
        return *this;
    }
};

template <typename T, typename U = T, typename V = T>
struct Vec3 {
    T x;
    U y;
    V z;

    Vec3() {
        if constexpr (std::is_pointer_v<T>) {
            using pointee = std::remove_pointer_t<T>;
            this->x = static_cast<T>(malloc(sizeof(pointee)));
            this->y = static_cast<U>(malloc(sizeof(pointee)));
            this->z = static_cast<V>(malloc(sizeof(pointee)));
        }
    };

    Vec3(T x, U y, V z) : x(x), y(y), z(z) {}
};
