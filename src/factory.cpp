#include "factory.hpp"
#include <algorithm>

namespace fmk {

Factory::Cache::ItemNodesT calculate_links(const Factory::MachinesT& machines);
Factory::Cache::QuantityPlotsT simulate_item_evolution(const Factory::ItemsT& items,
                                                       const Factory::MachinesT& machines,
                                                       const Factory::Cache::ItemNodesT& nodes,
                                                       std::size_t ticks_to_simulate);

Factory::Cache::Cache(const Factory& factory, std::size_t ticks_to_simulate) :
    _item_nodes(calculate_links(factory.machines)), _ticks_simulated(ticks_to_simulate) {
    for (auto& [item_uid, item] : factory.items) {
        switch (item.type) {
            case Item::NodeType::Input: {
                _inputs.emplace_back(item_uid);
            } break;

            case Item::NodeType::Output: {
                _outputs.emplace_back(item_uid);
            } break;

            default: break;
        }
    }

    _plots =
        simulate_item_evolution(factory.items, factory.machines, _item_nodes, ticks_to_simulate);
}

Factory::Cache::ItemNodesT calculate_links(const Factory::MachinesT& machines) {
    Factory::Cache::ItemNodesT result;

    for (const auto& [machine_uid, machine] : machines) {
        for (std::size_t input_i = 0; input_i < machine.inputs.size(); input_i++) {
            result[machine.inputs[input_i].item].inputs.insert(
                ItemNode::Link{machine_uid, input_i});
        }
        for (std::size_t output_i = 0; output_i < machine.outputs.size(); output_i++) {
            result[machine.outputs[output_i].item].outputs.insert(
                ItemNode::Link{machine_uid, output_i});
        }
    }

    return result;
}

Factory::Cache::QuantityPlotsT simulate_item_evolution(const Factory::ItemsT& items,
                                                       const Factory::MachinesT& machines,
                                                       const Factory::Cache::ItemNodesT& nodes,
                                                       std::size_t ticks_to_simulate) {
    Factory::Cache::QuantityPlotsT plots;
    plots.reserve(items.size());
    for (auto& [item_name, item] : items) {
        plots[item_name] = util::QuantityPlot(ticks_to_simulate, item.starting_quantity);
    }

    struct ProcessingTask {
        int starting_tick;
    };

    std::unordered_map<Uid, ProcessingTask> tasks;

    for (int tick = 0; tick < ticks_to_simulate; tick++) {
        // Process tasks
        for (auto task = tasks.cbegin(); task != tasks.cend();) {
            auto& machine = machines.at(task->first);

            // Check if the task has been finished
            if (tick >= task->second.starting_tick + machine.op_time.count()) {
                // Add outputs and remove this task if so
                for (auto& output : machine.outputs) {
                    plots.at(output.item).change_value(tick, output.quantity);
                }
                task = tasks.erase(task);
            } else {
                ++task;
            }
        }

        for (const auto& [machine_uid, machine] : machines) {
            // Check if this machine can do a processing cycle
            bool requirements_fulfilled = std::all_of(
                machine.inputs.begin(), machine.inputs.end(),
                [tick, &items, &plots](const ItemStream& required) -> bool {
                    auto& item = items.at(required.item);
                    return item.type == Item::NodeType::Input ||
                           plots.at(required.item).extrapolate_until(tick) >= required.quantity;
                });

            // Check if this machine is not currently busy with a previous cycle
            bool is_machine_free = tasks.find(machine_uid) == tasks.end();

            if (is_machine_free && requirements_fulfilled) {
                // Remove items required
                for (auto& input : machine.inputs) {
                    plots.at(input.item).change_value(tick, -input.quantity);
                }

                // Add processing task
                tasks[machine_uid] = ProcessingTask{tick};
            }
        }
    }

    for (auto& [_, plot] : plots) { plot.extrapolate_until(ticks_to_simulate); }

    return plots;
}

} // namespace fmk