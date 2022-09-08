#include "uid.hpp"
#include <cstdint>
#include <limits>
#include <plog/Log.h>

namespace fmk {

Uid UidPool::generate() {
    if (free_uids.empty()) {
        // Generate a new batch of free UIDs
        constexpr std::size_t BATCH_SIZE = 25;
        free_uids.reserve(BATCH_SIZE);
        for (int i = BATCH_SIZE; i >= 0; i--) { free_uids.emplace_back(leading_value + i); }
        leading_value += BATCH_SIZE;
    }
    auto value = free_uids.back();
    free_uids.pop_back();
    PLOGD << "Generated UID " << value.value;
    return value;
}

} // namespace fmk