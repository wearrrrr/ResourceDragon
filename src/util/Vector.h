#include <string>
#include <vector>
#include <algorithm>

template <typename T = std::string>
bool VectorHas(std::vector<T> vec, T item) {
    return std::find(vec.begin(), vec.end(), item) != vec.end();
}
