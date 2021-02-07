#pragma once

#include <TextEditor.h>
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
    void update_program_editor();
    void update_item_displayer();
    void parse_program();

    struct Cache {
        Factory::Cache factory_cache;
        std::vector<ImVec2> machine_positions;

        /// Whether if what is being drawn (the cache and factory) doesn't match up with the text
        /// entered (the program).
        mutable bool is_dirty = false;
    } cache;

    Factory factory;
    imnodes::EditorContext* imnodes_ctx;
    TextEditor program_editor;
    std::optional<PositionedMachine> new_machine;
    bool program_changed_this_frame = false;
};

} // namespace fmk