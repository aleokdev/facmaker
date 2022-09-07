#pragma once

#include <imgui.h>
#include <optional>
#include <utility>
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

    struct MachineEditor {
        MachineEditor(Machine machine, Uid machine_uid) :
            machine(std::move(machine)), machine_uid(machine_uid) {}
        /// The machine being edited.
        Machine machine;
        /// The UID of the machine being edited.
        Uid machine_uid;
    };

private:
    void update_processing_graph();
    void update_item_statistics();

    void parse_factory_json(std::istream& input);
    void output_factory_json(std::ostream& output) const;

    void regenerate_cache();

    struct Cache {
        Factory::Cache factory_cache;
    } cache;

    struct LogMessage {
        enum class Severity { Info, Warning, Error } severity;
        std::string message;
    };

    Factory factory;
    UidPool uid_pool;
    imnodes::EditorContext* imnodes_ctx;
    std::optional<MachineEditor> new_machine;
    bool show_imgui_demo_window = false;
    bool show_implot_demo_window = false;
};

} // namespace fmk