#include "util/quantity_plot.hpp"

namespace fmk::util {

QuantityPlot::QuantityPlot() {}
QuantityPlot::QuantityPlot(std::size_t capacity) { _container.reserve(capacity); }
QuantityPlot::QuantityPlot(std::size_t capacity, int starting_val) {
    _container.reserve(capacity);
    _container.emplace_back(starting_val);
}

void QuantityPlot::change_value(std::size_t tick, int mod) {
    if (_container.size() <= tick) {
        extrapolate_until(tick);
    }

    _container[tick] += mod;
}

void QuantityPlot::extrapolate_until(std::size_t tick) {
    auto last_element = _container.size() == 0 ? 0 : _container.back();
    for (std::size_t i = _container.size(); i <= tick; i++) {
        _container.emplace_back(last_element);
    }
}

const QuantityPlot::ContainerT& QuantityPlot::container() const { return _container; }

} // namespace fmk::util