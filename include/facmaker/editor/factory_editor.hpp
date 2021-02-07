#pragma once

#include <imgui.h>
#include <optional>
#include <vector>

#include "factory.hpp"

namespace imnodes {

struct EditorContext;

}
namespace fmk {

struct PositionedMachine {
    Machine machine;
    ImVec2 position;
};

class FactoryEditor {
public:
    FactoryEditor();
    ~FactoryEditor();

    void draw();

private:
    void update_processing_graph();
    void update_item_displayer();
    void parse_factory_json(std::istream& input);

    struct Cache {
        Factory::Cache factory_cache;
        std::vector<ImVec2> machine_positions;
    } cache;

    struct LogMessage {
        enum class Severity { Info, Warning, Error } severity;
        std::string message;
    };

    Factory factory;
    std::vector<LogMessage> logs;
    imnodes::EditorContext* imnodes_ctx;
    std::optional<PositionedMachine> new_machine;
};

} // namespace fmk