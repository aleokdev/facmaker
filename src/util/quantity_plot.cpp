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

    auto new_value = _container[tick] += mod;

    if (_max_value_i == static_cast<std::size_t>(-1)) {
        _max_value_i = tick;
    } else if (new_value > _container[_max_value_i]) {
        _max_value_i = tick;
    }
}

int QuantityPlot::extrapolate_until(std::size_t tick) {
    auto last_element = _container.size() == 0 ? 0 : _container.back();
    for (std::size_t i = _container.size(); i <= tick; i++) {
        _container.emplace_back(last_element);
    }
    return last_element;
}

} // namespace fmk::util