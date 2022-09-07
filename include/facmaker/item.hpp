#pragma once

#include <functional>
#include <string>

#include "uid.hpp"
#include "util/quantity_plot.hpp"

namespace fmk {

class QuantityGraph;

struct Item {
    enum class NodeType { Input, Output, Internal } type = NodeType::Internal;
    int starting_quantity = 0;
    std::string name;
    /// The ID of the output/input attribute of the item's node (Only relevant if it is an input or
    /// output).
    Uid attribute_uid{std::numeric_limits<int>::min()};
};

struct ItemStream {
    Uid item;
    int quantity;
    Uid uid;
};

} // namespace fmk
