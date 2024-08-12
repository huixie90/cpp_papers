#pragma once

#include <ranges>
#include <string>
#include <vector>

#include "any_view.hpp"

namespace lib {

struct Widget {
  std::string name;
  int size;
};

struct UI1 {
  std::vector<Widget> widgets_;

  std::ranges::any_view<std::string> getWidgetNames();
};

struct UI2 {
  std::vector<Widget> widgets_;
  // cannot use lambda because we need to spell the type of the view
  struct FilterFn {
    bool operator()(const Widget&) const;
  };

  struct TransformFn {
    std::string& operator()(Widget&) const;
  };
  std::ranges::transform_view<
      std::ranges::filter_view<std::ranges::ref_view<std::vector<Widget>>,
                               FilterFn>,
      TransformFn>
  getWidgetNames();
};

struct UI3 {
  std::vector<Widget> widgets_;

  std::vector<std::string> getWidgetNames() const;
};

struct UI4 {
  std::vector<Widget> widgets_;

  std::vector<std::reference_wrapper<const std::string>> getWidgetNames() const;
};



}  // namespace lib
