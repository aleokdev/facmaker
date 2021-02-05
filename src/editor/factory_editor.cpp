#include <boost/json.hpp>
#include <imgui.h>
#include <imnodes.h>
#include <implot.h>
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

FactoryEditor::FactoryEditor() : factory({}, {}, 0) {
    imnodes_ctx = imnodes::EditorContextCreate();
    program_editor.SetText(std::string(starting_program.data()));
    program_editor.SetShowWhitespaces(false);
    parse_program();

    imnodes::EditorContextSet(imnodes_ctx);
    imnodes::EditorContextResetPanning(ImVec2{50, 50});
}

void FactoryEditor::draw() {
    update_processing_graph();
    update_program_editor();
}

namespace {

void draw_item_graph(const Factory& factory, Item::NameT item_name, bool expanded = true) {
    ImPlot::SetNextPlotLimits(1, factory.ticks_simulated() + 1, 0, 10,
                              expanded ? ImGuiCond_Once : ImGuiCond_Always);
    double ticks_x[] = {1, static_cast<double>(factory.ticks_simulated()) / 2,
                        static_cast<double>(factory.ticks_simulated())};
    ImPlot::SetNextPlotTicksX(ticks_x, 3);
    double ticks_y[] = {0, 5, 10};
    ImPlot::SetNextPlotTicksY(ticks_y, 3);
    ImPlot::PushStyleColor(ImPlotCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
    ImPlot::PushStyleVar(ImPlotStyleVar_LabelPadding, ImVec2(0.75f, 1));
    ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(expanded ? 10 : 0, 5));
    if (ImPlot::BeginPlot(
            item_name.c_str(), "Tick", "Items", expanded ? ImVec2(400, 200) : ImVec2(100, 50),
            (expanded ? 0 : ImPlotFlags_NoChild) | ImPlotFlags_CanvasOnly ^ ImPlotFlags_NoTitle,
            expanded ? 0 : ImPlotAxisFlags_NoDecorations, expanded ? 0 : ImPlotAxisFlags_NoLabel)) {
        auto plot_size = factory.items().at(item_name).quantity_graph.container().size();

        // Shift the X axis one value to the left so that the total tick count equals the
        // last value plotted
        std::vector<int> plot_x(plot_size);
        for (std::size_t i = 1; i <= plot_size; i++) { plot_x[i - 1] = i; }

        ImPlot::PlotStairs(item_name.c_str(), plot_x.data(),
                           factory.items().at(item_name).quantity_graph.container().data(),
                           plot_size);

        ImPlot::EndPlot();
    }
    ImPlot::PopStyleVar();
    ImPlot::PopStyleVar();
    ImPlot::PopStyleColor();
}

void draw_factory_inputs(const Factory& factory,
                         int* next_uid,
                         std::unordered_map<Item::NameT, int>& item_uids) {
    for (auto& input : factory.inputs()) {
        imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar,
                                0xff + ((*next_uid * 50) % 0xFF << 8) |
                                    ((*next_uid * 186) % 0xFF << 16) |
                                    ((*next_uid * 67) % 0xFF << 24));
        imnodes::BeginNode((*next_uid)++);
        imnodes::SetNodeGridSpacePos(*next_uid - 1,
                                     ImVec2{0, static_cast<float>(*next_uid - 1) * 50.f});

        imnodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("Input");
        imnodes::EndNodeTitleBar();

        item_uids[std::string(input)] = *next_uid;
        imnodes::BeginOutputAttribute((*next_uid)++);
        ImGui::Text("%s", input.c_str());
        imnodes::EndOutputAttribute();

        imnodes::EndNode();

        imnodes::PopColorStyle();
    }
}

void draw_factory_machines(const Factory& factory,
                           int* next_uid,
                           std::vector<int>& factory_input_uids,
                           std::vector<int>& factory_output_uids) {
    for (auto& machine : factory.machines()) {
        imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar,
                                0xff + ((*next_uid * 50) % 0xFF << 8) |
                                    ((*next_uid * 186) % 0xFF << 16) |
                                    ((*next_uid * 67) % 0xFF << 24));
        imnodes::BeginNode((*next_uid)++);
        imnodes::SetNodeGridSpacePos(*next_uid - 1,
                                     ImVec2{0, static_cast<float>(*next_uid - 1) * 50.f});

        imnodes::BeginNodeTitleBar();
        ImGui::TextUnformatted(machine.name.c_str());
        imnodes::EndNodeTitleBar();

        for (auto input : machine.inputs) {
            factory_input_uids.emplace_back(*next_uid);
            imnodes::BeginInputAttribute((*next_uid)++);
            ImGui::Text("%i %s", input.quantity, input.item.c_str());
            imnodes::EndInputAttribute();
        }

        for (auto output : machine.outputs) {
            factory_output_uids.emplace_back(*next_uid);
            imnodes::BeginOutputAttribute((*next_uid)++);
            ImGui::Indent(40);
            ImGui::Text("%i %s", output.quantity, output.item.c_str());
            imnodes::EndOutputAttribute();
        }

        ImGui::TextDisabled("%li t/op", machine.op_time.count());

        imnodes::EndNode();

        imnodes::PopColorStyle();
    }
}

void draw_factory_outputs(const Factory& factory,
                          int* next_uid,
                          std::unordered_map<Item::NameT, int>& item_uids) {
    for (auto& output : factory.outputs()) {
        imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar,
                                0xff + ((*next_uid * 50) % 0xFF << 8) |
                                    ((*next_uid * 186) % 0xFF << 16) |
                                    ((*next_uid * 67) % 0xFF << 24));
        imnodes::BeginNode((*next_uid)++);
        imnodes::SetNodeGridSpacePos(*next_uid - 1,
                                     ImVec2{0, static_cast<float>(*next_uid - 1) * 50.f});

        imnodes::BeginNodeTitleBar();
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
        bool expand_graph;
        if ((expand_graph = ImGui::TreeNode("Output"))) {
            ImGui::TreePop();
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        imnodes::EndNodeTitleBar();

        item_uids[std::string(output)] = *next_uid;
        imnodes::BeginInputAttribute((*next_uid)++);
        draw_item_graph(factory, output, expand_graph);
        imnodes::EndInputAttribute();

        imnodes::EndNode();

        imnodes::PopColorStyle();
    }
}

void draw_factory_links(const Factory& factory,
                        int* next_uid,
                        std::vector<int>& factory_input_uids,
                        std::vector<int>& factory_output_uids,
                        std::unordered_map<Item::NameT, int>& item_uids) {
    for (auto& [item_name, node] : factory.item_nodes()) {
        for (auto& input : node.inputs) {
            for (auto& output : node.outputs) {
                imnodes::Link((*next_uid)++, factory_input_uids[input],
                              factory_output_uids[output]);
            }
        }

        if (factory.items().at(item_name).type == Item::NodeType::Input) {
            for (auto& input : node.inputs) {
                imnodes::Link((*next_uid)++, factory_input_uids[input], item_uids[item_name]);
            }
        } else if (factory.items().at(item_name).type == Item::NodeType::Output) {
            for (auto& output : node.outputs) {
                imnodes::Link((*next_uid)++, item_uids[item_name], factory_output_uids[output]);
            }
        }
    }
}

} // namespace

void FactoryEditor::update_processing_graph() {
    ImGui::SetNextWindowSize(ImVec2{500, 500}, ImGuiCond_Appearing);
    ImGui::Begin("Factory Displayer");

    if (cache.is_dirty) {
        imnodes::PushColorStyle(imnodes::ColorStyle_GridBackground, 0x2e2e2eff);
    }
    imnodes::BeginNodeEditor();

    std::vector<int> factory_input_uids;
    std::vector<int> factory_output_uids;
    std::unordered_map<Item::NameT, int> item_uids;
    int next_uid = 1;

    draw_factory_inputs(factory, &next_uid, item_uids);
    draw_factory_machines(factory, &next_uid, factory_input_uids, factory_output_uids);
    draw_factory_outputs(factory, &next_uid, item_uids);
    draw_factory_links(factory, &next_uid, factory_input_uids, factory_output_uids, item_uids);

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
    TextEditor::ErrorMarkers errors;
    Factory::MachinesT parsed_machines;
    Factory::ItemsT parsed_items;
    std::size_t ticks_to_simulate = 6000;
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
                                            parsed_items.insert({std::string(input_item), Item()});
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
                                            parsed_items.insert({std::string(output_item), Item()});
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

                            parsed_machines.emplace_back(result);
                        } else {
                            errors.insert({1, "Machines must be JSON objects"});
                        }
                    }
                } else {
                    errors.insert({1, "\"machines\" must be an array"});
                }
            }
            if (auto ticks_val = obj->if_contains("simulate")) {
                if (auto ticks = ticks_val->if_int64()) {
                    ticks_to_simulate = *ticks;
                } else {
                    errors.insert({1, "\"simulate\" value must be an integer"});
                }
            }
        } else {
            errors.insert({1, "Program must start with a JSON object"});
        }
    }

    program_editor.SetErrorMarkers(errors);

    if (errors.empty()) {
        factory = Factory(std::move(parsed_items), std::move(parsed_machines), ticks_to_simulate);
        cache.is_dirty = false;
    }
}

} // namespace fmk