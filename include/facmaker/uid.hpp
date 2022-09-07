#pragma once
#include <functional>
#include <limits>

namespace fmk {

struct Uid {
    static constexpr int INVALID_VALUE = std::numeric_limits<int>::min();

    constexpr Uid(int val) : value(val) {}
    Uid() = delete;

    // This is currently limited by imnodes
    int value;

    bool operator==(Uid const& other) const { return value == other.value; }
};

class UidPool {
public:
    UidPool(Uid next_uid) : next_uid(next_uid) {}

    Uid generate();

    std::size_t available() const;

private:
    Uid next_uid = {std::numeric_limits<int>::min()};
};

} // namespace fmk

template<> struct std::hash<fmk::Uid> {
    std::size_t operator()(fmk::Uid const& uid) const noexcept {
        return std::hash<unsigned long long>{}(uid.value);
    }
};