#include <string>
#include <unordered_set>
#include <vector>

#include "item.hpp"

namespace fmk {

struct Machine {
    std::string name;

    std::vector<ItemStream> inputs;
    std::vector<ItemStream> outputs;
};

struct Factory {
    std::vector<Machine> machines;
    std::unordered_set<Item> items;
};

} // namespace fmk