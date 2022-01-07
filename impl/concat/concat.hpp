#include <ranges>

namespace std::ranges {
template <range... R>
struct concat_view {
    int* begin() { return nullptr;}
    int* end() {return nullptr;}
};
} // namespace std::ranges
