#pragma once

#include <imgui.h>
#include <optional>
#include <vector>

#include "factory.hpp"

namespace imnodes {

struct EditorContext;

}
namespace fmk {

class FactoryEditor {
public:
    FactoryEditor();
    ~FactoryEditor();

    void draw();

private:
    void update_processing_graph();
    void update_item_displayer();

    void parse_factory_json(std::istream& input);
    void output_factory_json(std::ostream& output);

    void regenerate_cache();

    struct Cache {
        Factory::Cache factory_cache;
    } cache;

    struct LogMessage {
        enum class Severity { Info, Warning, Error } severity;
        std::string message;
    };

    Factory factory;
    imnodes::EditorContext* imnodes_ctx;
    std::optional<Machine> new_machine;
};

} // namespace fmk