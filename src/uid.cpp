#include "uid.hpp"
#include <cstdint>
#include <limits>

namespace fmk {

Uid UidPool::generate() {
    Uid to_return = next_uid;
    next_uid.value++;
    return to_return;
}

std::size_t UidPool::available() const {
    return static_cast<std::size_t>(std::numeric_limits<int>::max()) * 2 -
           (static_cast<std::int64_t>(next_uid.value) +
            static_cast<std::int64_t>(std::numeric_limits<int>::max()));
}

} // namespace fmk