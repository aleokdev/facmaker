#include "editor/factory_editor.hpp"

#include <algorithm>
#include <boost/json.hpp>
#include <fmt/core.h>
#include <fstream>
#include <imgui.h>
#include <imnodes.h>
#include <implot.h>
#include <iostream>
#include <optional>
#include <plog/Log.h>
#include <string>
#include <string_view>

#include "editor/draw_helpers.hpp"
#include "util/more_imgui.hpp"

namespace json = boost::json;

namespace fmk {

FactoryEditor::FactoryEditor() : factory{{}, {}} {
    imnodes_ctx = imnodes::EditorContextCreate();
    auto input_stream = std::ifstream("assets/starting_program.json");
    parse_factory_json(input_stream);

    imnodes::EditorContextSet(imnodes_ctx);
    imnodes::EditorContextResetPanning(ImVec2{50, 50});
}

FactoryEditor::~FactoryEditor() { imnodes::EditorContextFree(imnodes_ctx); }

void FactoryEditor::draw() {
    update_processing_graph();
    update_item_statistics();
}

void FactoryEditor::update_processing_graph() {
    ImGui::SetNextWindowSize(ImVec2{500, 500}, ImGuiCond_Appearing);
    ImGui::Begin("Factory Displayer", nullptr, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::MenuItem("Import From Clipboard")) {
            auto input = std::istringstream(ImGui::GetClipboardText());
            parse_factory_json(input);
            PLOGD << "Imported clipboard data";
        }
        if (ImGui::MenuItem("Export To Clipboard")) {
            auto output = std::ostringstream();
            output_factory_json(output);
            ImGui::SetClipboardText(output.str().c_str());
            PLOGD << "Exported to clipboard";
        }
        if (ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem("Show ImGui Demo Window", nullptr, &show_imgui_demo_window);
            ImGui::MenuItem("Show ImPlot Demo Window", nullptr, &show_implot_demo_window);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    if (show_imgui_demo_window) {
        ImGui::ShowDemoWindow(&show_imgui_demo_window);
    }
    if (show_implot_demo_window) {
        ImPlot::ShowDemoWindow(&show_implot_demo_window);
    }

    imnodes::BeginNodeEditor();
    ImVec2 editor_pos = ImGui::GetCursorScreenPos();

    static std::optional<ImVec2> editor_node_start_pos;
    FactoryIoUidMapT factory_input_uids;
    FactoryIoUidMapT factory_output_uids;
    std::unordered_map<Item::NameT, int> item_uids;
    auto machine_to_erase = factory.machines.cend();
    int last_output_id;

    draw_factory_inputs(factory, cache.factory_cache, item_uids);
    draw_factory_machines(factory, cache.factory_cache, factory_input_uids, factory_output_uids,
                          machine_to_erase);
    draw_factory_outputs(factory, cache.factory_cache, item_uids, &last_output_id);
    draw_factory_links(factory, cache.factory_cache, &last_output_id, factory_input_uids,
                       factory_output_uids, item_uids);

    if (new_machine) {
        if (draw_machine_editor(factory, *new_machine, &last_output_id, editor_node_start_pos)) {
            factory.machines.emplace_back(std::move(*new_machine));
            new_machine.reset();
            regenerate_cache();
        }
        editor_node_start_pos.reset();
    } else if (machine_to_erase != factory.machines.end()) {
        factory.machines.erase(machine_to_erase);

        regenerate_cache();
    }

    imnodes::EndNodeEditor();

    if (int start_attr; imnodes::IsLinkDropped(&start_attr)) {
        [&]() {
            for (std::size_t machine_i = 0; machine_i < factory_input_uids.size(); machine_i++) {
                const auto& machine_uids = factory_input_uids[machine_i];

                for (std::size_t input_i = 0; input_i < machine_uids.size(); input_i++) {
                    if (start_attr == machine_uids[input_i]) {
                        // Convert this machine's input item into an input!
                        factory.items[factory.machines[machine_i].inputs[input_i].item].type =
                            Item::NodeType::Input;

                        regenerate_cache();
                        return;
                    }
                }
            }

            for (std::size_t machine_i = 0; machine_i < factory_output_uids.size(); machine_i++) {
                const auto& machine_uids = factory_output_uids[machine_i];

                for (std::size_t output_i = 0; output_i < machine_uids.size(); output_i++) {
                    if (start_attr == machine_uids[output_i]) {
                        // Convert this machine's output item into an output!
                        factory.items[factory.machines[machine_i].outputs[output_i].item].type =
                            Item::NodeType::Output;

                        regenerate_cache();
                        return;
                    }
                }
            }
        }();
    }

    if (ImGui::BeginPopupContextItem("_ngc")) {
        if (ImGui::MenuItem("New Machine")) {
            std::string new_machine_name = "Machine";
            new_machine_name.reserve(32);
            new_machine.emplace(Machine{
                std::move(new_machine_name),
            });

            editor_node_start_pos = ImVec2{
                ImGui::GetMousePos().x - editor_pos.x - imnodes::EditorContextGetPanning().x,
                ImGui::GetMousePos().y - editor_pos.y - imnodes::EditorContextGetPanning().y};
        }
        ImGui::EndPopup();
    }

    if (ImGui::Begin("Factory Debug")) {
        ImGui::Text("Inputs");
        for (const auto& input : cache.factory_cache.inputs()) {
            ImGui::TextDisabled("%s", input.c_str());
        }
        ImGui::Text("Outputs");
        for (const auto& output : cache.factory_cache.outputs()) {
            ImGui::TextDisabled("%s", output.c_str());
        }
        ImGui::End();
    }

    ImGui::End();
}

void FactoryEditor::update_item_statistics() {
    ImGui::Begin("Item Statistics");
    for (auto& [item_name, _] : factory.items) {
        draw_item_graph(factory, cache.factory_cache, item_name, true);
    }
    ImGui::End();
}

void FactoryEditor::regenerate_cache() {
    cache.factory_cache = factory.generate_cache(cache.factory_cache.ticks_simulated());
}

void FactoryEditor::parse_factory_json(std::istream& input) {
    Factory::MachinesT parsed_machines;
    Factory::ItemsT parsed_items;
    std::size_t ticks_to_simulate = 6000;
    bool had_errors = false;

    json::error_code parse_error;
    json::stream_parser parser;
    std::size_t line_i = 0;
    for (std::string line; !parse_error && std::getline(input, line); line_i++) {
        parser.write_some(line, parse_error);
    }
    parser.finish(parse_error);

    if (parse_error) {
        PLOG_ERROR << fmt::format("JSON parsing error on line {}: {}", line_i,
                                  parse_error.message());
        had_errors = true;
    } else {
        auto val = parser.release();
        if (auto obj = val.if_object()) {
            if (auto inputs_val = obj->if_contains("inputs")) {
                if (auto inputs = inputs_val->if_object()) {
                    for (const auto& [input_name, _] : *inputs) {
                        parsed_items[std::string(input_name)].type = Item::NodeType::Input;
                    }
                }
            }
            if (auto outputs_val = obj->if_contains("outputs")) {
                if (auto outputs = outputs_val->if_object()) {
                    for (const auto& [output_name, _] : *outputs) {
                        parsed_items[std::string(output_name)].type = Item::NodeType::Output;
                    }
                }
            }

            if (auto machines_val = obj->if_contains("machines")) {
                if (auto machines = machines_val->if_array()) {
                    for (auto machine_val : *machines) {
                        if (auto machine = machine_val.if_object()) {
                            Machine result;

                            if (auto name_val = machine->if_contains("name")) {
                                if (auto name = name_val->if_string()) {
                                    result.name = *name;
                                } else {
                                    PLOG_ERROR
                                        << "JSON loading error: Machine names must be strings";
                                    had_errors = true;
                                }
                            } else {
                                PLOG_ERROR
                                    << "JSON loading error: Machines must have a \"name\" value";
                                had_errors = true;
                            }

                            if (auto time_val = machine->if_contains("time")) {
                                if (auto time = time_val->if_int64()) {
                                    result.op_time = util::ticks(*time);
                                } else {
                                    PLOG_ERROR << "JSON loading error: Machine operation times "
                                                  "must be integers";
                                    had_errors = true;
                                }
                            } else {
                                PLOG_ERROR
                                    << "JSON loading error: Machines must have a \"time\" value";
                                had_errors = true;
                            }

                            if (auto inputs_val = machine->if_contains("inputs")) {
                                if (auto inputs = inputs_val->if_object()) {
                                    for (const auto& [input_item, input_qty] : *inputs) {
                                        if (auto quantity = input_qty.if_int64()) {
                                            parsed_items.insert({std::string(input_item), Item()});
                                            result.inputs.emplace_back(
                                                ItemStream{std::string(input_item.data()),
                                                           static_cast<int>(*quantity)});
                                        } else {
                                            PLOG_ERROR << "JSON loading error: Input quantities "
                                                          "must be integers";
                                            had_errors = true;
                                        }
                                    }
                                } else {
                                    PLOG_ERROR
                                        << "JSON loading error: Machine inputs must be objects";
                                    had_errors = true;
                                }
                            } else {
                                PLOG_ERROR
                                    << "JSON loading error: Machines must have an \"inputs\" value";
                                had_errors = true;
                            }

                            if (auto outputs_val = machine->if_contains("outputs")) {
                                if (auto outputs = outputs_val->if_object()) {
                                    for (const auto& [output_item, output_qty] : *outputs) {
                                        if (auto quantity = output_qty.if_int64()) {
                                            parsed_items.insert({std::string(output_item), Item()});
                                            result.outputs.emplace_back(
                                                ItemStream{std::string(output_item.data()),
                                                           static_cast<int>(*quantity)});
                                        } else {
                                            PLOG_ERROR << "JSON loading error: Output quantities "
                                                          "must be integers";
                                            had_errors = true;
                                        }
                                    }
                                } else {
                                    PLOG_ERROR
                                        << "JSON loading error: Machine outputs must be objects";
                                    had_errors = true;
                                }
                            } else {
                                PLOG_ERROR << "JSON loading error: Machines must have an "
                                              "\"outputs\" value";
                                had_errors = true;
                            }

                            parsed_machines.emplace_back(result);
                        } else {
                            PLOG_ERROR << "JSON loading error: Machines must be JSON objects";
                            had_errors = true;
                        }
                    }
                } else {
                    PLOG_ERROR << "JSON loading error: \"machines\" must be an array";
                    had_errors = true;
                }
            }
            if (auto ticks_val = obj->if_contains("simulate")) {
                if (auto ticks = ticks_val->if_int64()) {
                    ticks_to_simulate = *ticks;
                } else {
                    PLOG_ERROR << "JSON loading error: \"simulate\" value must be an integer";
                    had_errors = true;
                }
            } else {
                PLOG_WARNING << "JSON loading warning: \"simulate\" value not "
                                "present, using the default value of 6000 ticks";
            }
        } else {
            PLOG_ERROR << "JSON loading error: Program must start with a JSON object";
            had_errors = true;
        }
    }

    if (!had_errors) {
        factory = Factory{std::move(parsed_items), std::move(parsed_machines)};
        cache.factory_cache = factory.generate_cache(ticks_to_simulate);
    }
}

void FactoryEditor::output_factory_json(std::ostream& out) {
    out << "{";

    // Inputs
    {
        out << "\"inputs\":{";
        const auto& inputs = cache.factory_cache.inputs();
        for (std::size_t i = 0; i < inputs.size(); i++) {
            out << "\"" << inputs[i] << "\":0";
            if (i < inputs.size() - 1) {
                out << ",";
            }
        }
        out << "},";
    }

    // Machines
    {
        out << "\"machines\":[";
        const auto& machines = factory.machines;
        for (std::size_t i = 0; i < machines.size(); i++) {
            out << "{";
            {
                const auto& machine = machines[i];
                out << "\"name\":\"" << machine.name << "\",";
                out << "\"inputs\":{";
                {
                    const auto& inputs = machine.inputs;
                    for (std::size_t i = 0; i < inputs.size(); i++) {
                        out << "\"" << inputs[i].item << "\":" << inputs[i].quantity;
                        if (i < inputs.size() - 1) {
                            out << ",";
                        }
                    }
                }
                out << "},";
                out << "\"outputs\":{";
                {
                    const auto& outputs = machine.outputs;
                    for (std::size_t i = 0; i < outputs.size(); i++) {
                        out << "\"" << outputs[i].item << "\":" << outputs[i].quantity;
                        if (i < outputs.size() - 1) {
                            out << ",";
                        }
                    }
                }
                out << "},";
                out << "\"time\":" << machine.op_time.count();
            }
            out << "}";
            if (i < machines.size() - 1) {
                out << ",";
            }
        }
        out << "],";
    }

    // Outputs
    {
        out << "\"outputs\":{";
        const auto& outputs = cache.factory_cache.outputs();
        for (std::size_t i = 0; i < outputs.size(); i++) {
            out << "\"" << outputs[i] << "\":0";
            if (i < outputs.size() - 1) {
                out << ",";
            }
        }
        out << "},";
    }

    // Simulate value
    { out << "\"simulate\":" << cache.factory_cache.ticks_simulated(); }

    out << "}";
}

} // namespace fmk