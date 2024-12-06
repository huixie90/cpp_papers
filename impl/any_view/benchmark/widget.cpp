#include "widget.hpp"

#include <ranges>

namespace lib {

std::ranges::any_view<std::string> UI1::getWidgetNames() {
  return widgets_ | std::views::filter([](const Widget& widget) {
           return widget.size > 10;
         }) |
         std::views::transform(&Widget::name);
}

bool UI2::FilterFn::operator()(const Widget& widget) const {
  return widget.size > 10;
}

std::string& UI2::TransformFn::operator()(Widget& widget) const {
  return widget.name;
}

std::ranges::transform_view<
    std::ranges::filter_view<std::ranges::ref_view<std::vector<Widget>>,
                             UI2::FilterFn>,
    UI2::TransformFn>
UI2::getWidgetNames() {
  return widgets_ | std::views::filter(UI2::FilterFn{}) |
         std::views::transform(UI2::TransformFn{});
}

std::vector<std::string> UI3::getWidgetNames() const {
  std::vector<std::string> results;
  // todo reserve? but how many?
  for (const Widget& widget : widgets_) {
    if (widget.size > 10) {
      results.push_back(widget.name);
    }
  }
  return results;
}

std::vector<std::string> UI3B::getWidgetNames() const {
  std::vector<std::string> results;
  results.reserve(widgets_.size());
  for (const Widget& widget : widgets_) {
    if (widget.size > 10) {
      results.push_back(widget.name);
    }
  }
  return results;
}

std::vector<std::string> UI3C::getWidgetNames() const {
  return widgets_ |
         std::views::filter([](const Widget& w) { return w.size > 10; }) |
         std::views::transform(&Widget::name) | std::ranges::to<std::vector>();
}

std::vector<std::reference_wrapper<const std::string>> UI4::getWidgetNames()
    const {
  return widgets_ | std::views::filter([](const Widget& widget) {
           return widget.size > 10;
         }) |
         std::views::transform(
             [](const Widget& widget) { return std::cref(widget.name); }) |
         std::ranges::to<std::vector>();
}

}  // namespace lib
