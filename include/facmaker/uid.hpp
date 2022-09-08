#pragma once
#include <functional>
#include <limits>

namespace fmk {

struct Uid {
    static constexpr int INVALID_VALUE = 0;

    explicit constexpr Uid(unsigned int val) : value(val) {}
    Uid() = delete;

    unsigned int value;

    bool operator==(Uid const& other) const { return value == other.value; }
};

class UidPool {
public:
    explicit UidPool(Uid next_uid) : next_uid(next_uid) {}

    Uid generate();

    [[nodiscard]] std::size_t available() const;
    [[nodiscard]] Uid get_next_uid() const { return next_uid; }

private:
    Uid next_uid{1};
};

} // namespace fmk

template<> struct std::hash<fmk::Uid> {
    std::size_t operator()(fmk::Uid const& uid) const noexcept {
        return std::hash<unsigned long long>{}(uid.value);
    }
};