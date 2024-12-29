#pragma once

#include <string>
#include <vector>
#include "any_view.hpp"

namespace lib {

int algo1(const std::vector<std::string>& strings);
int algo2(std::ranges::any_view<std::string> strings);

}