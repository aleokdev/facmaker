#include <boost/json.hpp>
#include <imgui.h>
#include <imnodes.h>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include "editor/factory_editor.hpp"

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
                "Crushed Iron Ore": 1
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
        }
    ],
    "outputs": {
        "Iron Ingot": 0
    }
}
)";

namespace fmk {

FactoryEditor::FactoryEditor() {
    imnodes_ctx = imnodes::EditorContextCreate();
    program_editor.SetText(std::string(starting_program.data()));
    program_editor.SetShowWhitespaces(false);
    parse_program();
}

void FactoryEditor::draw() {
    update_processing_graph();
    update_program_editor();
}

void FactoryEditor::update_processing_graph() {

    ImGui::SetNextWindowSize(ImVec2{500, 500}, ImGuiCond_Appearing);
    ImGui::Begin("Factory Displayer");

    if (cache.is_dirty) {
        imnodes::PushColorStyle(imnodes::ColorStyle_GridBackground, 0x2e2e2eff);
    }
    imnodes::BeginNodeEditor();
    imnodes::EditorContextResetPanning(ImVec2{50, 50});

    std::unordered_map<Item::NameT, std::vector<int>> item_inputs;
    std::unordered_map<Item::NameT, std::vector<int>> item_outputs;

    int next_uid = 1;
    for (auto machine : factory.machines) {
        imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar, 0xff + ((next_uid * 50) % 0xFF << 8) |
                                                                  ((next_uid * 186) % 0xFF << 16) |
                                                                  ((next_uid * 67) % 0xFF << 24));
        imnodes::BeginNode(next_uid++);
        imnodes::SetNodeGridSpacePos(next_uid - 1,
                                     ImVec2{0, static_cast<float>(next_uid - 1) * 50.f});

        imnodes::BeginNodeTitleBar();
        ImGui::TextUnformatted(machine.name.c_str());
        imnodes::EndNodeTitleBar();

        for (auto input : machine.inputs) {
            item_inputs[input.item].emplace_back(next_uid);
            imnodes::BeginInputAttribute(next_uid++);
            ImGui::Text("%i %s", input.quantity, input.item.c_str());
            imnodes::EndInputAttribute();
        }

        for (auto output : machine.outputs) {
            item_outputs[output.item].emplace_back(next_uid);
            imnodes::BeginOutputAttribute(next_uid++);
            ImGui::Indent(40);
            ImGui::Text("%i %s", output.quantity, output.item.c_str());
            imnodes::EndOutputAttribute();
        }

        ImGui::TextDisabled("%li t/op", machine.op_time.count());

        imnodes::EndNode();

        imnodes::PopColorStyle();
    }

    for (auto [item, input_ids] : item_inputs) {
        for (auto input : input_ids) {
            for (auto output : item_outputs[item]) { imnodes::Link(next_uid++, input, output); }
        }
    }

    imnodes::EndNodeEditor();
    if (cache.is_dirty) {
        imnodes::PopColorStyle();
    }

    ImGui::End();
}

void FactoryEditor::update_program_editor() {
    ImGui::Begin("Factory Programmer");
    program_editor.Render("text_editor");
    if (program_editor.IsTextChanged()) {
        parse_program();
    }
    ImGui::End();
}

void FactoryEditor::parse_program() {
    Factory parsed_fac;
    TextEditor::ErrorMarkers errors;
    cache.is_dirty = true;

    json::error_code parse_error;
    std::istringstream ssinput = std::istringstream(program_editor.GetText());
    json::stream_parser parser;
    std::size_t line_i = 0;
    for (std::string line; !parse_error && std::getline(ssinput, line); line_i++) {
        parser.write_some(line, parse_error);
    }
    parser.finish(parse_error);

    if (parse_error) {
        errors.insert({line_i, parse_error.message()});
    } else {
        auto val = parser.release();
        if (auto obj = val.if_object()) {
            if (auto machines_val = obj->if_contains("machines")) {
                if (auto machines = machines_val->if_array()) {
                    for (auto machine_val : *machines) {
                        if (auto machine = machine_val.if_object()) {
                            Machine result;

                            if (auto name_val = machine->if_contains("name")) {
                                if (auto name = name_val->if_string()) {
                                    result.name = *name;
                                } else {
                                    errors.insert({1, "Machine names must be strings"});
                                }
                            } else {
                                errors.insert({1, "Machines must have a \"name\" value"});
                            }

                            if (auto time_val = machine->if_contains("time")) {
                                if (auto time = time_val->if_int64()) {
                                    result.op_time = util::ticks(*time);
                                } else {
                                    errors.insert({1, "Machine operation times must be integers"});
                                }
                            } else {
                                errors.insert({1, "Machines must have a \"time\" value"});
                            }

                            if (auto inputs_val = machine->if_contains("inputs")) {
                                if (auto inputs = inputs_val->if_object()) {
                                    for (const auto [input_item, input_qty] : *inputs) {
                                        if (auto quantity = input_qty.if_int64()) {
                                            result.inputs.emplace_back(
                                                ItemStream{std::string(input_item.data()),
                                                           static_cast<int>(*quantity)});
                                        } else {
                                            errors.insert({1, "Input quantities must be integers"});
                                        }
                                    }
                                } else {
                                    errors.insert({1, "Machine inputs must be objects"});
                                }
                            } else {
                                errors.insert({1, "Machines must have an \"inputs\" value"});
                            }

                            if (auto outputs_val = machine->if_contains("outputs")) {
                                if (auto outputs = outputs_val->if_object()) {
                                    for (const auto [output_item, output_qty] : *outputs) {
                                        if (auto quantity = output_qty.if_int64()) {
                                            result.outputs.emplace_back(
                                                ItemStream{std::string(output_item.data()),
                                                           static_cast<int>(*quantity)});
                                        } else {
                                            errors.insert(
                                                {1, "Output quantities must be integers"});
                                        }
                                    }
                                } else {
                                    errors.insert({1, "Machine outputs must be objects"});
                                }
                            } else {
                                errors.insert({1, "Machines must have an \"outputs\" value"});
                            }

                            parsed_fac.machines.emplace_back(result);
                        } else {
                            errors.insert({1, "Machines must be JSON objects"});
                        }
                    }
                } else {
                    errors.insert({1, "\"machines\" must be an array"});
                }
            }
        } else {
            errors.insert({1, "Program must start with a JSON object"});
        }
    }

    program_editor.SetErrorMarkers(errors);

    if (errors.empty()) {
        factory = parsed_fac;
        cache.is_dirty = false;
    }
}

} // namespace fmk