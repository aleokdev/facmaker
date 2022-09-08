#pragma once

#include <imgui.h>
#include <imgui_node_editor.h>
#include <optional>
#include <utility>
#include <vector>

#include "factory.hpp"

namespace fmk {

class FactoryEditor {
public:
    FactoryEditor();
    ~FactoryEditor();

    void draw();

    struct MachineEditor {
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

    Factory factory;
    UidPool uid_pool;
    ax::NodeEditor::EditorContext* node_editor_ctx;
    std::optional<MachineEditor> new_machine;
    bool show_imgui_demo_window = false;
    bool show_implot_demo_window = false;
};

} // namespace fmk