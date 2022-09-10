#include "editor/factory_editor.hpp"

#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <imgui.h>
#include <imgui_node_editor.h>
#include <implot.h>
#include <optional>
#include <plog/Log.h>
#include <string>
#include <string_view>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "editor/draw_helpers.hpp"
#include "pfd/pfd.hpp"

namespace ed = ax::NodeEditor;

namespace fmk {

void EditorContextDeleter::operator()(ax::NodeEditor::EditorContext* ptr) {
    if (ed::GetCurrentEditor() == ptr) {
        ed::SetCurrentEditor(nullptr);
    }
    ed::DestroyEditor(ptr);
}

ed::EditorContext* create_editor() {
    ed::Config config;
    // al: This is technically UB, but also how we're supposed to block imgui-node-editor from
    // saving to a file as far as I know
    config.SettingsFile = nullptr;

    return ed::CreateEditor(&config);
}

FactoryEditor::FactoryEditor() :
    uid_pool(Uid(Uid::INVALID_VALUE + 1)), node_editor_ctx(create_editor()) {

    ed::SetCurrentEditor(node_editor_ctx.get());
}

FactoryEditor::FactoryEditor(std::istringstream& to_parse) : FactoryEditor() {
    parse_factory_json(to_parse);
}

FactoryEditor& FactoryEditor::operator=(FactoryEditor&& rhs) noexcept {
    // I hate non-destructive moves I hate non-destructive moves I hate non-destructive moves
    factory = std::move(rhs.factory);
    cache = std::move(rhs.cache);
    uid_pool = std::move(rhs.uid_pool);
    node_editor_ctx = std::move(rhs.node_editor_ctx);
    new_machine = std::move(rhs.new_machine);
    path_being_edited = std::move(rhs.path_being_edited);
    ticks_to_simulate_on_regenerate = rhs.ticks_to_simulate_on_regenerate;
    show_imgui_demo_window = rhs.show_imgui_demo_window;
    show_implot_demo_window = rhs.show_implot_demo_window;

    return *this;
}

void FactoryEditor::draw(GLFWwindow* window) {
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    draw_main_menu_bar(window);
    draw_processing_graph(window);
    draw_item_statistics();
    draw_item_list();

    if (show_imgui_demo_window) {
        ImGui::ShowDemoWindow(&show_imgui_demo_window);
    }
    if (show_implot_demo_window) {
        ImPlot::ShowDemoWindow(&show_implot_demo_window);
    }
}

void FactoryEditor::set_path_being_edited(GLFWwindow* window, std::string&& str) {
    path_being_edited = str;
    if (str.empty()) {
        glfwSetWindowTitle(window, "facmaker");
    } else {
        glfwSetWindowTitle(window,
                           fmt::format("facmaker - {}",
                                       std::filesystem::path(path_being_edited).filename().c_str())
                               .c_str());
    }
}

// TODO: Move outside of FactoryEditor
void FactoryEditor::draw_main_menu_bar(GLFWwindow* window) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New")) {
                *this = std::move(FactoryEditor());
            }
            if (ImGui::MenuItem("Open...")) {
                auto selection = pfd::open_file("Open Factory").result();
                if (!selection.empty()) {
                    std::ifstream file(selection[0]);
                    parse_factory_json(file);
                    PLOGD << "Imported data from '" << selection[0] << "'";

                    set_path_being_edited(window, std::move(selection[0]));
                }
            }
            if (ImGui::MenuItem("Save", nullptr, false, !path_being_edited.empty())) {
                std::ofstream file(path_being_edited);
                output_factory_json(file);
                PLOGD << "Exported data to '" << path_being_edited << "'";
            }
            if (ImGui::MenuItem("Save As...")) {
                auto destination = pfd::save_file("Save Factory").result();
                if (!destination.empty()) {
                    std::ofstream file(destination);
                    output_factory_json(file);
                    PLOGD << "Exported data to '" << destination << "'";

                    set_path_being_edited(window, std::move(destination));
                }
            }
            if (ImGui::MenuItem("Import From Clipboard")) {
                auto input = std::istringstream(ImGui::GetClipboardText());
                parse_factory_json(input);
                PLOGD << "Imported clipboard data";
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem("Show ImGui Demo Window", nullptr, &show_imgui_demo_window);
            ImGui::MenuItem("Show ImPlot Demo Window", nullptr, &show_implot_demo_window);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void FactoryEditor::draw_processing_graph(GLFWwindow* window) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    if (ImGui::Begin("Node Editor")) {
        ImGui::PopStyleVar(3);

        ed::SetCurrentEditor(node_editor_ctx.get());
        ed::Begin("Factory Node Editor");
        static std::optional<ImVec2> editor_node_start_pos;
        auto machine_to_erase = factory.machines.cend();
        auto machine_to_edit = factory.machines.cend();

        if (const auto input_to_delete = draw_factory_inputs(factory, cache.factory_cache)) {
            factory.items.erase(*input_to_delete);
            regenerate_cache();
        }
        draw_factory_machines(factory, cache.factory_cache, machine_to_erase, machine_to_edit);
        if (const auto output_to_delete = draw_factory_outputs(factory, cache.factory_cache)) {
            factory.items.erase(*output_to_delete);
            regenerate_cache();
        }
        draw_factory_links(factory, cache.factory_cache, uid_pool);

        if (new_machine) {
            if (draw_machine_editor(factory, *new_machine, uid_pool, editor_node_start_pos)) {
                factory.machines[new_machine->machine_uid] = std::move(new_machine->machine);
                new_machine.reset();
                regenerate_cache();
            }
            editor_node_start_pos.reset();
        } else if (machine_to_erase != factory.machines.end()) {
            factory.machines.erase(machine_to_erase);

            regenerate_cache();
        } else if (machine_to_edit != factory.machines.cend()) {
            auto [uid, machine] = *machine_to_edit;

            factory.machines.erase(machine_to_edit);
            regenerate_cache();

            new_machine = MachineEditor{machine, uid};
        }

        ImVec2 open_popup_pos = ImGui::GetMousePos();
        ed::Suspend();
        if (ed::ShowBackgroundContextMenu()) {
            ImGui::OpenPopup("Background Context Menu");
        }

        if (ImGui::BeginPopup("Background Context Menu")) {
            const auto can_add_machines = !factory.items.empty();
            if (ImGui::MenuItem("New Machine", nullptr, false, can_add_machines)) {
                std::string new_machine_name = "Machine";
                new_machine_name.reserve(32);
                new_machine.emplace(
                    MachineEditor{Machine{std::move(new_machine_name)}, uid_pool.generate()});

                // We use open_popup_pos because it's in editor coordinates (Called
                // ImGui::GetMousePos() before ed::Suspend())
                ed::SetNodePosition(new_machine->machine_uid.value, open_popup_pos);
            }
            if (!can_add_machines && ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "Add at least one item via the Item List window before creating a machine");
            }
            ImGui::EndPopup();
        }
        ed::Resume();

        ed::End();
        ed::SetCurrentEditor(nullptr);

        ImGui::End();
    } else {
        ImGui::PopStyleVar(3);
    }
}

void FactoryEditor::draw_item_list() {
    if (ImGui::Begin("Item List")) {
        static auto flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
                            ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersOuter |
                            ImGuiTableFlags_BordersV | ImGuiTableFlags_SizingStretchProp |
                            ImGuiTableFlags_RowBg;

        static Uid item_being_edited(Uid::INVALID_VALUE);
        static std::string item_edit_name;
        static Item::NodeType item_edit_type;
        static int item_edit_starting_quantity;

        if (ImGui::BeginTable("item_table", 4, flags)) {
            ImGui::TableSetupColumn("Edit");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("Starting Quantity");
            ImGui::TableHeadersRow();
            for (const auto& [item_uid, item] : factory.items) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::Button(fmt::format("Edit##{}", item_uid.value).c_str())) {
                    item_being_edited = item_uid;
                    item_edit_name = item.name;
                    item_edit_type = item.type;
                    item_edit_starting_quantity = item.starting_quantity;
                    ImGui::OpenPopup("Edit Item");
                }
                ImGui::TableNextColumn();
                ImGui::Text("%s", item.name.data());
                ImGui::TableNextColumn();
                switch (item.type) {
                    case Item::NodeType::Input: ImGui::Text("Input"); break;
                    case Item::NodeType::Output: ImGui::Text("Output"); break;
                    case Item::NodeType::Internal: ImGui::Text("Internal"); break;
                }
                ImGui::TableNextColumn();
                ImGui::Text("%i", item.starting_quantity);
            }

            if (ImGui::BeginPopup("Edit Item")) {
                ImGui::InputText("Name", &item_edit_name);
                ImGui::Combo("Type", reinterpret_cast<int*>(&item_edit_type),
                             "Input\0Output\0Internal\0");
                ImGui::InputInt("Starting Quantity", &item_edit_starting_quantity);
                if (ImGui::Button("Cancel")) {
                    item_being_edited = Uid(Uid::INVALID_VALUE);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Apply")) {
                    auto& item = factory.items.at(item_being_edited);
                    item.name = item_edit_name;
                    item.type = item_edit_type;
                    item.starting_quantity = item_edit_starting_quantity;

                    item_being_edited = Uid(Uid::INVALID_VALUE);
                    ImGui::CloseCurrentPopup();
                    regenerate_cache();
                }
                ImGui::EndPopup();
            }

            ImGui::EndTable();
        }

        static char name[50];
        static int type;
        static int starting_quantity;
        ImGui::InputText("Name", name, sizeof(name));
        ImGui::Combo("Type", &type, "Input\0Output\0Internal");
        ImGui::InputInt("Starting Quantity", &starting_quantity);
        if (ImGui::Button("Create new item")) {
            factory.items[uid_pool.generate()] = Item{static_cast<Item::NodeType>(type),
                                                      starting_quantity, name, uid_pool.generate()};
            regenerate_cache();
        }

        ImGui::Text("Inputs");
        for (const auto& input : cache.factory_cache.inputs()) {
            ImGui::TextDisabled("%s", factory.items.at(input).name.c_str());
        }
        ImGui::Text("Outputs");
        for (const auto& output : cache.factory_cache.outputs()) {
            ImGui::TextDisabled("%s", factory.items.at(output).name.c_str());
        }

        ImGui::End();
    }
}

void FactoryEditor::draw_item_statistics() {
    if (ImGui::Begin("Item Statistics")) {
        int tts = static_cast<int>(ticks_to_simulate_on_regenerate);
        ImGui::SliderInt("Ticks To Simulate", &tts, 1000, 1000000);
        if (tts != ticks_to_simulate_on_regenerate) {
            ticks_to_simulate_on_regenerate = tts;
            regenerate_cache();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(%.2fs)", static_cast<float>(ticks_to_simulate_on_regenerate) / 20.f);
        for (auto& [item_name, _] : factory.items) {
            draw_item_graph(factory, cache.factory_cache, item_name, true);
        }
        ImGui::End();
    }
}

void FactoryEditor::regenerate_cache() {
    cache.factory_cache = factory.generate_cache(ticks_to_simulate_on_regenerate);
}

} // namespace fmk