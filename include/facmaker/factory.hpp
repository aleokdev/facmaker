#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "item.hpp"

namespace fmk {

namespace util {

using ticks = std::chrono::duration<std::int64_t, std::ratio<1, 20>>;

}

struct Machine {
    using IndexT = std::size_t;

    std::string name;

    std::vector<ItemStream> inputs;
    std::vector<ItemStream> outputs;
    util::ticks op_time;
};

struct ItemNode {
    std::unordered_set<Machine::IndexT> inputs;
    std::unordered_set<Machine::IndexT> outputs;
};

class Factory {
public:
    using MachinesT = std::vector<Machine>;
    using ItemNamesT = std::vector<Item::NameT>;
    using ItemsT = std::unordered_map<Item::NameT, Item>;
    using ItemNodesT = std::unordered_map<Item::NameT, ItemNode>;

    Factory(ItemsT&& items, MachinesT&& machines);

    /// The machines (processing nodes) in this factory.
    const MachinesT& machines() const { return _machines; }
    /// The items being processed in this factory.
    const ItemsT& items() const { return _items; }
    /// A generated container with all the input item names in this factory.
    const ItemNamesT& inputs() const { return _inputs; }
    /// A generated container with all the output item names in this factory.
    const ItemNamesT& outputs() const { return _outputs; }
    /// A generated map of the relationship of items with the machines in this factory.
    const ItemNodesT& item_nodes() const { return _item_nodes; }

private:
    MachinesT _machines;
    ItemsT _items;
    ItemNamesT _inputs;
    ItemNamesT _outputs;
    ItemNodesT _item_nodes;
};

} // namespace fmk