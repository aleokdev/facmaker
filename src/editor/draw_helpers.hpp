#include <imgui.h>
#include <imgui_internal.h>
#include <imnodes.h>
#include <implot.h>

#include "factory.hpp"
#include "util/more_imgui.hpp"

namespace fmk {

inline void draw_item_graph(const Factory& factory,
                            const Factory::Cache& cache,
                            Item::NameT item_name,
                            bool expanded = true,
                            bool reload_plot_limits = false) {
    auto& item = factory.items.at(item_name);
    auto& plot = cache.plots().at(item_name);

    ImPlot::SetNextPlotLimits(1, cache.ticks_simulated() + 1, 0, plot.max_value() + 1,
                              (expanded && !reload_plot_limits) ? ImGuiCond_Appearing
                                                                : ImGuiCond_Always);

    {
        double ticks_x[] = {1, static_cast<double>(cache.ticks_simulated()) / 2,
                            static_cast<double>(cache.ticks_simulated())};
        ImPlot::SetNextPlotTicksX(ticks_x, 3);
    }
    {
        double ticks_y[] = {0, static_cast<double>(plot.max_value()) / 2,
                            static_cast<double>(plot.max_value())};
        ImPlot::SetNextPlotTicksY(ticks_y, 3);
    }

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

inline void draw_factory_inputs(const Factory& _factory,
                                const Factory::Cache& cache,
                                std::unordered_map<Item::NameT, int>& item_uids) {
    int next_uid = 1;

    for (auto& input : cache.inputs()) {
        imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar, 0xff + ((next_uid * 50) % 0xFF << 8) |
                                                                  ((next_uid * 186) % 0xFF << 16) |
                                                                  ((next_uid * 67) % 0xFF << 24));
        imnodes::BeginNode(next_uid++);

        imnodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("Input");
        imnodes::EndNodeTitleBar();

        item_uids[std::string(input)] = next_uid;
        imnodes::BeginOutputAttribute(next_uid++);
        ImGui::Text("%s", input.c_str());
        imnodes::EndOutputAttribute();

        imnodes::EndNode();

        imnodes::PopColorStyle();
    }
}

using MachineIoUidMapT = std::vector<int>;
using FactoryIoUidMapT = std::vector<MachineIoUidMapT>;

inline void draw_factory_machines(const Factory& factory,
                                  const Factory::Cache& _cache,
                                  FactoryIoUidMapT& factory_input_uids,
                                  FactoryIoUidMapT& factory_output_uids,
                                  Factory::MachinesT::const_iterator& out_machine_to_erase) {
    int next_uid = std::numeric_limits<int>::max() / 3;

    factory_input_uids.resize(factory.machines.size());
    factory_output_uids.resize(factory.machines.size());
    for (std::size_t machine_i = 0; machine_i < factory.machines.size(); machine_i++) {
        auto& machine = factory.machines[machine_i];

        imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar, 0xff + ((next_uid * 50) % 0xFF << 8) |
                                                                  ((next_uid * 186) % 0xFF << 16) |
                                                                  ((next_uid * 67) % 0xFF << 24));
        imnodes::BeginNode(next_uid++);

        imnodes::BeginNodeTitleBar();
        if (ImGui::CloseButton(ImGui::GetID("delete"),
                               ImVec2{ImGui::GetCursorPosX() + 3, ImGui::GetCursorPosY() + 45})) {
            out_machine_to_erase = factory.machines.begin() + machine_i;
        }
        ImGui::Dummy(ImVec2{10, 0});
        ImGui::SameLine();
        ImGui::TextUnformatted(machine.name.c_str());
        ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x);
        imnodes::EndNodeTitleBar();

        for (auto& input : machine.inputs) {
            factory_input_uids[machine_i].emplace_back(next_uid);
            imnodes::BeginInputAttribute((next_uid)++);
            ImGui::Text("%i %s", input.quantity, input.item.c_str());
            imnodes::EndInputAttribute();
        }

        for (auto& output : machine.outputs) {
            factory_output_uids[machine_i].emplace_back(next_uid);
            imnodes::BeginOutputAttribute((next_uid)++);
            ImGui::Indent(40);
            ImGui::Text("%i %s", output.quantity, output.item.c_str());
            imnodes::EndOutputAttribute();
        }

        ImGui::TextDisabled("%li t/op", machine.op_time.count());

        imnodes::EndNode();

        imnodes::PopColorStyle();
    }
}

inline void draw_factory_outputs(const Factory& factory,
                                 const Factory::Cache& cache,
                                 std::unordered_map<Item::NameT, int>& item_uids,
                                 int* last_uid) {
    *last_uid = std::numeric_limits<int>::max() / 3 * 2;

    for (auto& output : cache.outputs()) {
        imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar,
                                0xff + ((*last_uid * 50) % 0xFF << 8) |
                                    ((*last_uid * 186) % 0xFF << 16) |
                                    ((*last_uid * 67) % 0xFF << 24));
        imnodes::BeginNode((*last_uid)++);

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

        item_uids[std::string(output)] = *last_uid;
        imnodes::BeginInputAttribute((*last_uid)++);
        draw_item_graph(factory, cache, output, expand_graph);
        imnodes::EndInputAttribute();

        imnodes::EndNode();

        imnodes::PopColorStyle();
    }
}

inline void draw_factory_links(const Factory& factory,
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

inline bool draw_machine_editor(const Factory& factory,
                                Machine& machine,
                                int* next_uid,
                                std::optional<ImVec2> node_pos = std::nullopt) {
    auto node_id = *next_uid;

    imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar, 0xff + ((*next_uid * 50) % 0xFF << 8) |
                                                              ((*next_uid * 186) % 0xFF << 16) |
                                                              ((*next_uid * 67) % 0xFF << 24));
    imnodes::BeginNode((*next_uid)++);
    if (node_pos.has_value())
        imnodes::SetNodeGridSpacePos(*next_uid - 1, *node_pos);

    imnodes::BeginNodeTitleBar();
    ImGui::SelectableInput("##name", false, machine.name.data(), machine.name.capacity());
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

    // Draw inputs
    machine.inputs.erase(
        std::remove_if(machine.inputs.begin(), machine.inputs.end(),
                       [&factory, &draw_io_manip, next_uid](ItemStream& input) -> bool {
                           imnodes::BeginInputAttribute((*next_uid)++);
                           bool remove = draw_io_manip(input);
                           imnodes::EndInputAttribute();
                           return remove;
                       }),
        machine.inputs.end());
    if (ImGui::SmallButton("+##add_input")) {
        machine.inputs.emplace_back(ItemStream{factory.items.begin()->first, 1});
    }

    // Draw outputs
    machine.outputs.erase(
        std::remove_if(machine.outputs.begin(), machine.outputs.end(),
                       [&factory, &draw_io_manip, next_uid](ItemStream& output) -> bool {
                           imnodes::BeginOutputAttribute((*next_uid)++);
                           ImGui::Indent(40);
                           bool remove = draw_io_manip(output);
                           ImGui::Unindent();
                           imnodes::EndOutputAttribute();
                           return remove;
                       }),
        machine.outputs.end());

    ImGui::Indent(40);
    if (ImGui::SmallButton("+##add_output")) {
        machine.outputs.emplace_back(ItemStream{factory.items.begin()->first, 1});
    }
    ImGui::Unindent();

    ImGui::SetNextItemWidth(50);
    int op_time = machine.op_time.count();
    ImGui::DragInt("##tpop", &op_time, 1.f, 1, 99999);
    machine.op_time = util::ticks(op_time);
    ImGui::SameLine();
    ImGui::TextDisabled("t/op");

    bool is_finished = ImGui::Button("Finish");

    imnodes::EndNode();

    imnodes::PopColorStyle();

    return is_finished;
}

} // namespace fmk