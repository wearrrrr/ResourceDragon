#include "int.h"
#include <cstdlib>

template <typename T>
T* malloc(usize data_size) {
    return static_cast<T*>(malloc(data_size));
}
