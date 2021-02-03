#include <string>
#include <unordered_set>
#include <vector>

#include "item.hpp"
#pragma once
#include <chrono>
#include <string>
#include <unordered_set>
#include <vector>

namespace fmk {

namespace util {

using ticks = std::chrono::duration<std::int64_t, std::ratio<1, 20>>;

}

struct Machine {
    std::string name;

    std::vector<ItemStream> inputs;
    std::vector<ItemStream> outputs;
    util::ticks op_time;
};

struct Factory {
    std::vector<Machine> machines;
    std::vector<Item::NameT> inputs;
    std::vector<Item::NameT> outputs;

    std::unordered_set<Item> items;
};

} // namespace fmk