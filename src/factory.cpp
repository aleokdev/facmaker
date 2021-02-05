#include "factory.hpp"
#include <algorithm>
#include <iostream>

namespace fmk {

Factory::ItemNodesT calculate_links(const Factory::MachinesT& machines);
void simulate_item_evolution(Factory::ItemsT& items,
                             const Factory::MachinesT& machines,
                             const Factory::ItemNodesT& nodes,
                             int ticks_to_simulate);

Factory::Factory(ItemsT&& items, MachinesT&& machines, std::size_t ticks_to_simulate) :
    _items(std::move(items)), _machines(std::move(machines)),
    _item_nodes(calculate_links(_machines)), _ticks_simulated(ticks_to_simulate) {
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

    simulate_item_evolution(_items, _machines, _item_nodes, ticks_to_simulate);
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

void simulate_item_evolution(Factory::ItemsT& items,
                             const Factory::MachinesT& machines,
                             const Factory::ItemNodesT& nodes,
                             int ticks_to_simulate) {
    for (auto& [_, item] : items) {
        item.quantity_graph = util::QuantityPlot(ticks_to_simulate, item.starting_quantity);
    }

    struct ProcessingTask {
        int starting_tick;
    };

    std::unordered_map<Machine::IndexT, ProcessingTask> tasks;

    for (int tick = 0; tick < ticks_to_simulate; tick++) {
        // Process tasks
        for (auto task = tasks.begin(); task != tasks.end();) {
            auto& machine = machines[task->first];

            // Check if the task has been finished
            if (tick >= task->second.starting_tick + machine.op_time.count()) {
                // Add outputs and remove this task if so
                for (auto& output : machine.outputs) {
                    items.at(output.item).quantity_graph.change_value(tick, output.quantity);
                }
                task = tasks.erase(task);
            } else {
                ++task;
            }
        }

        for (Machine::IndexT machine_i = 0; machine_i < machines.size(); machine_i++) {
            auto& machine = machines[machine_i];

            // Check if this machine can do a processing cycle
            bool requirements_fulfilled =
                std::all_of(machine.inputs.begin(), machine.inputs.end(),
                            [tick, &items](const ItemStream& required) -> bool {
                                auto& item = items.at(required.item);
                                return item.type == Item::NodeType::Input ||
                                       required.quantity >
                                           items.at(required.item).quantity_graph.container()[tick];
                            });

            // Check if this machine is not currently busy with a previous cycle
            bool is_machine_free = tasks.find(machine_i) == tasks.end();

            if (is_machine_free && requirements_fulfilled) {
                // Remove items required
                for (auto& input : machine.inputs) {
                    items.at(input.item).quantity_graph.change_value(tick, -input.quantity);
                }

                // Add processing task
                tasks[machine_i] = ProcessingTask{tick};
            }
        }
    }

    for (auto& item : items) { item.second.quantity_graph.extrapolate_until(ticks_to_simulate); }
}

} // namespace fmk