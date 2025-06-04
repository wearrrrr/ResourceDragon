#include <type_traits>
#include <cstdlib>

#pragma once

template <typename T>
struct Vec2 {
    T x;
    T y;

    Vec2() {
        if constexpr (std::is_pointer_v<T>) {
            using pointee = std::remove_pointer_t<T>;
            this->x = static_cast<T>(malloc(sizeof(pointee)));
            this->y = static_cast<T>(malloc(sizeof(pointee)));
        }
    };
};
