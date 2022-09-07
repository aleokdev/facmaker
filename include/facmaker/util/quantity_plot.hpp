#pragma once

#include <vector>

namespace fmk::util {

class QuantityPlot {
public:
    using ContainerT = std::vector<int>;

    QuantityPlot() = default;
    explicit QuantityPlot(std::size_t capacity);
    QuantityPlot(std::size_t capacity, int starting_val);

    /// Changes a value in the plot by a modifier.
    /// If the value already exists, the modifier is directly applied as `val += mod`. If
    /// the value doesn't exist, the plot will be extended until `tick` using the last value
    /// in it, and then the same `val += mod` change will be applied.
    void change_value(std::size_t tick, int modifier);

    /// Inserts values coming from the last available one until `tick`.
    /// @returns The element extrapolated.
    int extrapolate_until(std::size_t tick);

    const ContainerT& container() const { return _container; }
    int max_value() const { return _max_value_i == -1 ? 0 : _container[_max_value_i]; }

private:
    ContainerT _container;
    std::size_t _max_value_i = -1;
};

} // namespace fmk::util