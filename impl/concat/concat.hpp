#include <ranges>

namespace std::ranges {
template <range... R>
struct concat_view {
    int* begin() {}
    int* end() {}
};
} // namespace std::ranges
