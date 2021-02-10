#include "editor/factory_editor.hpp"

#include <algorithm>
#include <boost/json.hpp>
#include <fmt/core.h>
#include <imgui.h>
#include <imnodes.h>
#include <implot.h>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

#include "editor/draw_helpers.hpp"
#include "util/more_imgui.hpp"

namespace json = boost::json;

constexpr std::string_view starting_program = R"({
    "inputs": {
        "Iron Ore": 0
    },
    "start_with": {
        "Iron Ore": 64
    },
    "machines": [
        {
            "name": "Macerator",
            "inputs": {
                "Iron Ore": 1
            },
            "outputs": {
                "Crushed Iron Ore": 2
            },
            "time": 200
        },
        {
            "name": "Furnace",
            "inputs": {
                "Crushed Iron Ore": 1
            },
            "outputs": {
                "Iron Ingot": 1
            },
            "time": 200
        },
        {
        	"name": "Test",
        	"inputs": {},
        	"outputs": {
        		"Test": 1
        	},
        	"time": 1
        },
        {
        	"name": "Test2",
        	"inputs": {
                "Test": 1
            },
        	"outputs": {
        		"Foo": 1
        	},
        	"time": 1
        }
    ],
    "outputs": {
        "Iron Ingot": 0,
        "Test": 0
    },
    "simulate": 10
}
)";

namespace fmk {

FactoryEditor::FactoryEditor() : factory{{}, {}} {
    imnodes_ctx = imnodes::EditorContextCreate();
    auto input_stream = std::istringstream(std::string(starting_program));
    parse_factory_json(input_stream);

    imnodes::EditorContextSet(imnodes_ctx);
    imnodes::EditorContextResetPanning(ImVec2{50, 50});
}

FactoryEditor::~FactoryEditor() { imnodes::EditorContextFree(imnodes_ctx); }

void FactoryEditor::draw() {
    update_processing_graph();
    update_item_displayer();
}

void FactoryEditor::update_processing_graph() {
    static std::optional<ImVec2> editor_node_start_pos;

    ImGui::SetNextWindowSize(ImVec2{500, 500}, ImGuiCond_Appearing);
    ImGui::Begin("Factory Displayer", nullptr, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::Selectable("Import From Clipboard")) {
            auto input = std::istringstream(ImGui::GetClipboardText());
            parse_factory_json(input);
        }
        ImGui::EndMenuBar();
    }

    imnodes::BeginNodeEditor();
    ImVec2 editor_pos = ImGui::GetCursorScreenPos();

    FactoryIoUidMapT factory_input_uids;
    FactoryIoUidMapT factory_output_uids;
    std::unordered_map<Item::NameT, int> item_uids;
    Factory::MachinesT::const_iterator machine_to_erase = factory.machines.end();
    int next_uid = 1;

    draw_factory_inputs(factory, cache.factory_cache, &next_uid, item_uids);
    draw_factory_machines(factory, cache.factory_cache, &next_uid, factory_input_uids,
                          factory_output_uids, machine_to_erase);
    draw_factory_outputs(factory, cache.factory_cache, &next_uid, item_uids);
    draw_factory_links(factory, cache.factory_cache, &next_uid, factory_input_uids,
                       factory_output_uids, item_uids);

    if (new_machine) {
        if (draw_machine_editor(factory, *new_machine, &next_uid, editor_node_start_pos)) {
            factory.machines.emplace_back(std::move(*new_machine));
            new_machine.reset();
            cache.factory_cache = factory.generate_cache(cache.factory_cache.ticks_simulated());
        }
        editor_node_start_pos.reset();
    } else if (machine_to_erase != factory.machines.end()) {
        factory.machines.erase(machine_to_erase);
        cache.factory_cache = factory.generate_cache(cache.factory_cache.ticks_simulated());
    }

    imnodes::EndNodeEditor();

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

    ImGui::End();
}

void FactoryEditor::update_item_displayer() {
    ImGui::Begin("Item Displayer");
    for (auto& [item_name, _] : factory.items) {
        draw_item_graph(factory, cache.factory_cache, item_name, true);
    }
    ImGui::End();
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
        logs.emplace_back(
            LogMessage{LogMessage::Severity::Error, fmt::format("JSON parsing error on line {}: {}",
                                                                line_i, parse_error.message())});
        had_errors = true;
    } else {
        auto val = parser.release();
        if (auto obj = val.if_object()) {
            if (auto inputs_val = obj->if_contains("inputs")) {
                if (auto inputs = inputs_val->if_object()) {
                    for (const auto [input, _] : *inputs) {
                        parsed_items[std::string(input)].type = Item::NodeType::Input;
                    }
                }
            }
            if (auto outputs_val = obj->if_contains("outputs")) {
                if (auto outputs = outputs_val->if_object()) {
                    for (const auto [output, _] : *outputs) {
                        parsed_items[std::string(output)].type = Item::NodeType::Output;
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
                                    logs.emplace_back(LogMessage{
                                        LogMessage::Severity::Error,
                                        "JSON loading error: Machine names must be strings"});
                                    had_errors = true;
                                }
                            } else {
                                logs.emplace_back(LogMessage{
                                    LogMessage::Severity::Error,
                                    "JSON loading error: Machines must have a \"name\" value"});
                                had_errors = true;
                            }

                            if (auto time_val = machine->if_contains("time")) {
                                if (auto time = time_val->if_int64()) {
                                    result.op_time = util::ticks(*time);
                                } else {
                                    logs.emplace_back(
                                        LogMessage{LogMessage::Severity::Error,
                                                   "JSON loading error: Machine operation times "
                                                   "must be integers"});
                                    had_errors = true;
                                }
                            } else {
                                logs.emplace_back(LogMessage{
                                    LogMessage::Severity::Error,
                                    "JSON loading error: Machines must have a \"time\" value"});
                                had_errors = true;
                            }

                            if (auto inputs_val = machine->if_contains("inputs")) {
                                if (auto inputs = inputs_val->if_object()) {
                                    for (const auto [input_item, input_qty] : *inputs) {
                                        if (auto quantity = input_qty.if_int64()) {
                                            parsed_items.insert({std::string(input_item), Item()});
                                            result.inputs.emplace_back(
                                                ItemStream{std::string(input_item.data()),
                                                           static_cast<int>(*quantity)});
                                        } else {
                                            logs.emplace_back(
                                                LogMessage{LogMessage::Severity::Error,
                                                           "JSON loading error: Input quantities "
                                                           "must be integers"});
                                            had_errors = true;
                                        }
                                    }
                                } else {
                                    logs.emplace_back(LogMessage{
                                        LogMessage::Severity::Error,
                                        "JSON loading error: Machine inputs must be objects"});
                                    had_errors = true;
                                }
                            } else {
                                logs.emplace_back(LogMessage{
                                    LogMessage::Severity::Error,
                                    "JSON loading error: Machines must have an \"inputs\" value"});
                                had_errors = true;
                            }

                            if (auto outputs_val = machine->if_contains("outputs")) {
                                if (auto outputs = outputs_val->if_object()) {
                                    for (const auto [output_item, output_qty] : *outputs) {
                                        if (auto quantity = output_qty.if_int64()) {
                                            parsed_items.insert({std::string(output_item), Item()});
                                            result.outputs.emplace_back(
                                                ItemStream{std::string(output_item.data()),
                                                           static_cast<int>(*quantity)});
                                        } else {
                                            logs.emplace_back(
                                                LogMessage{LogMessage::Severity::Error,
                                                           "JSON loading error: Output quantities "
                                                           "must be integers"});
                                            had_errors = true;
                                        }
                                    }
                                } else {
                                    logs.emplace_back(LogMessage{
                                        LogMessage::Severity::Error,
                                        "JSON loading error: Machine outputs must be objects"});
                                    had_errors = true;
                                }
                            } else {
                                logs.emplace_back(LogMessage{
                                    LogMessage::Severity::Error,
                                    "JSON loading error: Machines must have an \"outputs\" value"});
                                had_errors = true;
                            }

                            parsed_machines.emplace_back(result);
                        } else {
                            logs.emplace_back(
                                LogMessage{LogMessage::Severity::Error,
                                           "JSON loading error: Machines must be JSON objects"});
                            had_errors = true;
                        }
                    }
                } else {
                    logs.emplace_back(
                        LogMessage{LogMessage::Severity::Error,
                                   "JSON loading error: \"machines\" must be an array"});
                    had_errors = true;
                }
            }
            if (auto ticks_val = obj->if_contains("simulate")) {
                if (auto ticks = ticks_val->if_int64()) {
                    ticks_to_simulate = *ticks;
                } else {
                    logs.emplace_back(
                        LogMessage{LogMessage::Severity::Error,
                                   "JSON loading error: \"simulate\" value must be an integer"});
                    had_errors = true;
                }
            } else {
                logs.emplace_back(LogMessage{LogMessage::Severity::Warning,
                                             "JSON loading warning: \"simulate\" value not "
                                             "present, using the default value of 6000 ticks"});
            }
        } else {
            logs.emplace_back(
                LogMessage{LogMessage::Severity::Error,
                           "JSON loading error: Program must start with a JSON object"});
            had_errors = true;
        }
    }

    if (!had_errors) {
        factory = Factory{std::move(parsed_items), std::move(parsed_machines)};
        cache.factory_cache = factory.generate_cache(ticks_to_simulate);
    }
}

} // namespace fmk