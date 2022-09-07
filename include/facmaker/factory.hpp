#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "item.hpp"
#include "uid.hpp"
#include "util/quantity_plot.hpp"

namespace fmk {

namespace util {

using ticks = std::chrono::duration<int, std::ratio<1, 20>>;

}

struct Machine {
    std::string name;

    std::vector<ItemStream> inputs;
    std::vector<ItemStream> outputs;
    util::ticks op_time;
};

struct ItemNode {
    struct Link {
        Uid machine;
        std::size_t io_index;
    };

    struct LinkHash {
        std::size_t operator()(const ItemNode::Link& link) const noexcept {
            return link.io_index ^ (link.machine.value << 1);
        }
    };

    std::unordered_set<Link, LinkHash> inputs;
    std::unordered_set<Link, LinkHash> outputs;
};

inline bool operator==(const ItemNode::Link& a, const ItemNode::Link& b) {
    return a.machine == b.machine && a.io_index == b.io_index;
}

struct Factory;
struct Factory {
    using MachinesT = std::unordered_map<Uid, Machine>;
    using ItemsT = std::unordered_map<Uid, Item>;

    /// The items being processed in this factory.
    ItemsT items;
    /// The machines (processing nodes) in this factory.
    MachinesT machines;

    class Cache {
    public:
        using ItemUidsT = std::vector<Uid>;
        using ItemNodesT = std::unordered_map<Uid, ItemNode>;
        using QuantityPlotsT = std::unordered_map<Uid, util::QuantityPlot>;

        Cache() = default;

        /// A generated container with quantity plots for all the items in this factory.
        const QuantityPlotsT& plots() const { return _plots; }
        /// A generated container with all the input item names in this factory.
        const ItemUidsT& inputs() const { return _inputs; }
        /// A generated container with all the output item names in this factory.
        const ItemUidsT& outputs() const { return _outputs; }
        /// A generated map of the relationship of items with the machines in this factory.
        const ItemNodesT& item_nodes() const { return _item_nodes; }
        /// The amount of ticks simulated for the item processing.
        std::size_t ticks_simulated() const { return _ticks_simulated; }

    private:
        Cache(const Factory&, std::size_t ticks_to_simulate);
        friend class Factory;

        ItemUidsT _inputs;
        ItemUidsT _outputs;
        ItemNodesT _item_nodes;
        QuantityPlotsT _plots;
        std::size_t _ticks_simulated = 0;
    };

    Cache generate_cache(std::size_t ticks_to_simulate) const {
        return {*this, ticks_to_simulate};
    };
};

} // namespace fmk