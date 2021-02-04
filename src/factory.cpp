#include "factory.hpp"
#include <algorithm>
#include <iostream>

namespace fmk {

Factory::ItemNodesT calculate_links(const Factory::MachinesT& machines);

Factory::Factory(ItemsT&& items, MachinesT&& machines) :
    _items(std::move(items)), _machines(std::move(machines)),
    _item_nodes(calculate_links(_machines)) {
    for (auto& [item_name, item] : _items) {
        switch (item.type) {
            case Item::NodeType::Input: {
                _inputs.emplace_back(item_name);
            } break;

            case Item::NodeType::Output: {
                _outputs.emplace_back(item_name);
            } break;

            default: break;
        }
    }
}

Factory::ItemNodesT calculate_links(const Factory::MachinesT& machines) {
    Factory::ItemNodesT result;

    Machine::IndexT machine_i = 0;
    for (auto& machine : machines) {
        for (auto& input : machine.inputs) { result[input.item].inputs.insert(machine_i); }
        for (auto& output : machine.outputs) { result[output.item].outputs.insert(machine_i); }

        machine_i++;
    }

    return result;
}

} // namespace fmk