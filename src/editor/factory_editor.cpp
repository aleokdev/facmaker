#include "editor/factory_editor.hpp"

#include <algorithm>
#include <boost/json.hpp>
#include <fmt/core.h>
#include <imgui.h>
#include <imnodes.h>
#include <implot.h>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

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

namespace {

void draw_item_graph(const Factory& factory,
                     const Factory::Cache& cache,
                     Item::NameT item_name,
                     bool expanded = true,
                     bool reload_plot_limits = false) {
    auto& item = factory.items.at(item_name);
    auto& plot = cache.plots().at(item_name);

    ImPlot::SetNextPlotLimits(1, cache.ticks_simulated() + 1, 0, plot.max_value() + 1,
                              (expanded && !reload_plot_limits) ? ImGuiCond_Appearing
                                                                : ImGuiCond_Always);
    double ticks_x[] = {1, static_cast<double>(cache.ticks_simulated()) / 2,
                        static_cast<double>(cache.ticks_simulated())};
    ImPlot::SetNextPlotTicksX(ticks_x, 3);
    double ticks_y[] = {0, static_cast<double>(plot.max_value()) / 2,
                        static_cast<double>(plot.max_value())};
    ImPlot::SetNextPlotTicksY(ticks_y, 3);
    ImPlot::PushStyleColor(ImPlotCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
    ImPlot::PushStyleVar(ImPlotStyleVar_LabelPadding, ImVec2(0.75f, 1));
    ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(expanded ? 10 : 0, 5));
    if (ImPlot::BeginPlot(
            item_name.c_str(), "Tick", "Items", expanded ? ImVec2(400, 200) : ImVec2(100, 50),
            (expanded ? 0 : ImPlotFlags_NoChild) | ImPlotFlags_CanvasOnly ^ ImPlotFlags_NoTitle,
            expanded ? 0 : ImPlotAxisFlags_NoDecorations, expanded ? 0 : ImPlotAxisFlags_NoLabel)) {
        auto plot_size = plot.container().size();

        // Shift the X axis one value to the left so that the total tick count equals the
        // last value plotted
        std::vector<int> plot_x(plot_size);
        for (std::size_t i = 1; i <= plot_size; i++) { plot_x[i - 1] = i; }

        ImPlot::PlotStairs(item_name.c_str(), plot_x.data(), plot.container().data(), plot_size);

        ImPlot::EndPlot();
    }
    ImPlot::PopStyleVar();
    ImPlot::PopStyleVar();
    ImPlot::PopStyleColor();
}

void draw_factory_inputs(const Factory::Cache& cache,
                         int* next_uid,
                         std::unordered_map<Item::NameT, int>& item_uids) {
    for (auto& input : cache.inputs()) {
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

using MachineIoUidMapT = std::vector<int>;
using FactoryIoUidMapT = std::vector<MachineIoUidMapT>;

void draw_factory_machines(const Factory& factory,
                           int* next_uid,
                           FactoryIoUidMapT& factory_input_uids,
                           FactoryIoUidMapT& factory_output_uids) {
    factory_input_uids.resize(factory.machines.size());
    factory_output_uids.resize(factory.machines.size());
    for (std::size_t machine_i = 0; machine_i < factory.machines.size(); machine_i++) {
        auto& machine = factory.machines[machine_i];

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

        for (auto& input : machine.inputs) {
            factory_input_uids[machine_i].emplace_back(*next_uid);
            imnodes::BeginInputAttribute((*next_uid)++);
            ImGui::Text("%i %s", input.quantity, input.item.c_str());
            imnodes::EndInputAttribute();
        }

        for (auto& output : machine.outputs) {
            factory_output_uids[machine_i].emplace_back(*next_uid);
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
                          const Factory::Cache& cache,
                          int* next_uid,
                          std::unordered_map<Item::NameT, int>& item_uids) {
    for (auto& output : cache.outputs()) {
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
        draw_item_graph(factory, cache, output, expand_graph);
        imnodes::EndInputAttribute();

        imnodes::EndNode();

        imnodes::PopColorStyle();
    }
}

void draw_factory_links(const Factory& factory,
                        const Factory::Cache& cache,
                        int* next_uid,
                        const FactoryIoUidMapT& factory_input_uids,
                        const FactoryIoUidMapT& factory_output_uids,
                        std::unordered_map<Item::NameT, int>& item_uids) {
    for (auto& [item_name, node] : cache.item_nodes()) {
        for (auto& input : node.inputs) {
            for (auto& output : node.outputs) {
                imnodes::Link((*next_uid)++, factory_input_uids[input.machine][input.io_index],
                              factory_output_uids[output.machine][output.io_index]);
            }
        }

        auto& item = factory.items.at(item_name);

        if (item.type == Item::NodeType::Input) {
            for (auto& input : node.inputs) {
                imnodes::Link((*next_uid)++, factory_input_uids[input.machine][input.io_index],
                              item_uids[item_name]);
            }
        } else if (item.type == Item::NodeType::Output) {
            for (auto& output : node.outputs) {
                imnodes::Link((*next_uid)++, item_uids[item_name],
                              factory_output_uids[output.machine][output.io_index]);
            }
        }
    }
}

bool draw_machine_editor(const Factory& factory, PositionedMachine& m, int* next_uid) {
    imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar, 0xff + ((*next_uid * 50) % 0xFF << 8) |
                                                              ((*next_uid * 186) % 0xFF << 16) |
                                                              ((*next_uid * 67) % 0xFF << 24));
    imnodes::BeginNode((*next_uid)++);
    imnodes::SetNodeGridSpacePos(*next_uid - 1, m.position);

    imnodes::BeginNodeTitleBar();
    ImGui::SelectableInput("##name", false, m.machine.name.data(), m.machine.name.capacity());
    imnodes::EndNodeTitleBar();

    const auto& draw_io_manip = [&factory](ItemStream& obj) -> bool {
        ImGui::SetNextItemWidth(20);
        ImGui::DragInt("##o_quantity", &obj.quantity, 1.f, 1, 99);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(50);
        if (ImGui::BeginCombo("##o_item", obj.item.c_str())) {
            for (auto& [item_name, _] : factory.items) {
                const bool is_selected = obj.item == item_name;
                if (ImGui::Selectable(item_name.c_str(), is_selected)) {
                    obj.item = item_name;
                }

                // Set the initial focus when opening the combo (scrolling +
                // keyboard navigation focus)
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        return ImGui::SmallButton("-##rm_io");
    };

    m.machine.inputs.erase(
        std::remove_if(m.machine.inputs.begin(), m.machine.inputs.end(),
                       [&factory, &draw_io_manip, next_uid](ItemStream& input) -> bool {
                           imnodes::BeginInputAttribute((*next_uid)++);
                           bool remove = draw_io_manip(input);
                           imnodes::EndInputAttribute();
                           return remove;
                       }),
        m.machine.inputs.end());
    if (ImGui::SmallButton("+##add_input")) {
        m.machine.inputs.emplace_back(ItemStream{factory.items.begin()->first, 1});
    }

    m.machine.outputs.erase(
        std::remove_if(m.machine.outputs.begin(), m.machine.outputs.end(),
                       [&factory, &draw_io_manip, next_uid](ItemStream& output) -> bool {
                           imnodes::BeginOutputAttribute((*next_uid)++);
                           ImGui::Indent(40);
                           bool remove = draw_io_manip(output);
                           ImGui::Unindent();
                           imnodes::EndOutputAttribute();
                           return remove;
                       }),
        m.machine.outputs.end());

    ImGui::Indent(40);
    if (ImGui::SmallButton("+##add_output")) {
        m.machine.outputs.emplace_back(ItemStream{factory.items.begin()->first, 1});
    }
    ImGui::Unindent();

    ImGui::SetNextItemWidth(50);
    int op_time = m.machine.op_time.count();
    ImGui::DragInt("##tpop", &op_time, 1.f, 1, 99999);
    m.machine.op_time = util::ticks(op_time);
    ImGui::SameLine();
    ImGui::TextDisabled("t/op");

    bool is_finished = ImGui::Button("Finish");

    imnodes::EndNode();

    imnodes::PopColorStyle();

    return is_finished;
}

} // namespace

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
    ImGui::SetNextWindowSize(ImVec2{500, 500}, ImGuiCond_Appearing);
    ImGui::Begin("Factory Displayer");

    imnodes::BeginNodeEditor();
    ImVec2 editor_pos = ImGui::GetCursorScreenPos();

    FactoryIoUidMapT factory_input_uids;
    FactoryIoUidMapT factory_output_uids;
    std::unordered_map<Item::NameT, int> item_uids;
    int next_uid = 1;

    draw_factory_inputs(cache.factory_cache, &next_uid, item_uids);
    draw_factory_machines(factory, &next_uid, factory_input_uids, factory_output_uids);
    draw_factory_outputs(factory, cache.factory_cache, &next_uid, item_uids);
    draw_factory_links(factory, cache.factory_cache, &next_uid, factory_input_uids,
                       factory_output_uids, item_uids);
    if (new_machine) {
        if (draw_machine_editor(factory, *new_machine, &next_uid)) {
            factory.machines.emplace_back(std::move(new_machine->machine));
            new_machine.reset();
            cache.factory_cache = factory.generate_cache(cache.factory_cache.ticks_simulated());
        }
    }

    imnodes::EndNodeEditor();

    if (ImGui::BeginPopupContextItem("_ngc")) {
        if (ImGui::MenuItem("New Machine")) {
            std::string new_machine_name = "Machine";
            new_machine_name.reserve(32);
            new_machine.emplace(PositionedMachine{
                {std::move(new_machine_name)},
                ImVec2{ImGui::GetMousePos().x - editor_pos.x - imnodes::EditorContextGetPanning().x,
                       ImGui::GetMousePos().y - editor_pos.y -
                           imnodes::EditorContextGetPanning().y}});
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