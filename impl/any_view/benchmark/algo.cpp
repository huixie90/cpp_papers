#include "algo.hpp"

namespace lib {

int algo1(const std::vector<std::string>& strings) {
  int result = 0;
  for (const auto& str : strings) {
    if (str.size() > 6) {
      result += str.size();
    }
  }
  return result;
}

int algo2(std::ranges::any_view<std::string> strings)
{
  int result = 0;
  for (const auto& str : strings) {
    if (str.size() > 6) {
      result += str.size();
    }
  }
  return result;
}
}  // namespace lib