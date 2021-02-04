#pragma once

#include <functional>
#include <string>

namespace fmk {

struct Item {
    using NameT = std::string;

    enum class NodeType { Input, Output, Internal } type = NodeType::Internal;
    int starting_quantity = 0;
};

struct ItemStream {
    Item::NameT item;
    int quantity;
};

} // namespace fmk
