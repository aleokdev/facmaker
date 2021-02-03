#pragma once

#include <functional>
#include <string>

namespace fmk {

struct Item {
    using NameT = std::string;

    enum class NodeType { Input, Output, Internal } type;

    std::string name;
};

struct ItemStream {
    Item::NameT item;
    int quantity;
};

} // namespace fmk

namespace std {

template<> struct hash<::fmk::Item> {
    std::size_t operator()(::fmk::Item const& item) const noexcept {
        return std::hash<std::string>{}(item.name);
    }
};

} // namespace std