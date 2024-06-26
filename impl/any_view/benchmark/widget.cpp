#include "widget.hpp"

namespace lib {

std::ranges::any_view<const std::string&> UI1::getWidgetNames() const {
  return widgets_ | std::views::filter([](const Widget& widget) {
           return widget.size > 10;
         }) |
         std::views::transform(&Widget::name);
}

bool UI2::FilterFn::operator()(const Widget& widget) const {
  return widget.size > 10;
}

const std::string& UI2::TransformFn::operator()(const Widget& widget) const {
  return widget.name;
}

std::ranges::transform_view<
    std::ranges::filter_view<std::ranges::ref_view<const std::vector<Widget>>,
                             UI2::FilterFn>,
    UI2::TransformFn>
UI2::getWidgetNames() const {
  return widgets_ | std::views::filter(UI2::FilterFn{}) |
         std::views::transform(UI2::TransformFn{});
}

std::vector<std::string> UI3::getWidgetNames() const {
  return widgets_ | std::views::filter([](const Widget& widget) {
           return widget.size > 10;
         }) |
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
