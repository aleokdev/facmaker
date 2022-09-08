#pragma once

#include <functional>
#include <string>

#include "uid.hpp"
#include "util/quantity_plot.hpp"

namespace fmk {

struct Item {
    enum class NodeType : int { Input, Output, Internal } type = NodeType::Internal;
    int starting_quantity = 0;
    std::string name;
    /// The ID of the output/input attribute of the item's node (Only relevant if it is an input or
    /// output).
    Uid attribute_uid{Uid::INVALID_VALUE};
};

struct ItemStream {
    Uid item;
    int quantity;
    Uid uid;
};

} // namespace fmk
