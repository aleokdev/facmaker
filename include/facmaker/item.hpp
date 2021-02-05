#pragma once

#include <functional>
#include <string>

#include "util/quantity_plot.hpp"

namespace fmk {

class QuantityGraph;

struct Item {
    using NameT = std::string;

    enum class NodeType { Input, Output, Internal } type = NodeType::Internal;
    int starting_quantity = 0;

    util::QuantityPlot quantity_graph;
};

struct ItemStream {
    Item::NameT item;
    int quantity;
};

} // namespace fmk
