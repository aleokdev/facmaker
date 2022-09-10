#pragma once
#include <vector>

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
    explicit UidPool(Uid leading_value) : leading_value(leading_value.value) {}

    Uid generate();

    [[nodiscard]] unsigned int get_leading_value() const { return leading_value; }

private:
    std::vector<Uid> free_uids;
    unsigned int leading_value{1};
};

} // namespace fmk

template<> struct std::hash<fmk::Uid> {
    std::size_t operator()(fmk::Uid const& uid) const noexcept {
        return std::hash<unsigned long long>{}(uid.value);
    }
};