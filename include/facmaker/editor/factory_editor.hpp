#pragma once

#include <imgui.h>
#include <imgui_node_editor.h>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "factory.hpp"

struct GLFWwindow;

namespace fmk {

struct EditorContextDeleter {
    void operator()(ax::NodeEditor::EditorContext*);
};

class FactoryEditor {
public:
    FactoryEditor();
    FactoryEditor(const std::string& path);

    FactoryEditor& operator=(FactoryEditor&&) noexcept;

    FactoryEditor(const FactoryEditor&) = delete;
    FactoryEditor& operator=(const FactoryEditor&) = delete;

    void draw(GLFWwindow* window);

    struct MachineEditor {
        /// The machine being edited.
        Machine machine;
        /// The UID of the machine being edited.
        Uid machine_uid;
    };

private:
    void draw_processing_graph(GLFWwindow* window);
    void draw_item_statistics();

    void parse_factory_json(std::istream& input);
    void output_factory_json(std::ostream& output) const;

    void regenerate_cache();

    void set_path_being_edited(GLFWwindow*, std::string&&);

    struct Cache {
        Factory::Cache factory_cache;
    } cache;

    Factory factory;
    UidPool uid_pool;
    std::unique_ptr<ax::NodeEditor::EditorContext, EditorContextDeleter> node_editor_ctx;
    std::optional<MachineEditor> new_machine;
    std::string path_being_edited;
    std::size_t ticks_to_simulate_on_regenerate = 6000;
    bool show_imgui_demo_window = false;
    bool show_implot_demo_window = false;
};

} // namespace fmk