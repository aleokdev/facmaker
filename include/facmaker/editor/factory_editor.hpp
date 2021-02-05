#pragma once

#include <TextEditor.h>
#include <imgui.h>
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
    void update_program_editor();
    void update_item_displayer();
    void parse_program();

    struct Cache {
        std::vector<ImVec2> machine_positions;

        /// Whether if what is being drawn (the cache and factory) matches up with the text
        /// entered (the program).
        mutable bool is_dirty = true;
    } cache;

    Factory factory;
    imnodes::EditorContext* imnodes_ctx;
    TextEditor program_editor;
    bool program_changed_this_frame = false;
};

} // namespace fmk