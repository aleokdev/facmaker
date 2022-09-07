#include <imgui.h>
#include <imgui_internal.h>
#include <imnodes.h>
#include <implot.h>

#include "factory.hpp"
#include "util/more_imgui.hpp"

namespace fmk {

inline void draw_item_graph(const Factory& factory,
                            const Factory::Cache& cache,
                            const Uid item_uid,
                            bool expanded = true,
                            bool reload_plot_limits = false) {
    auto& item = factory.items.at(item_uid);
    auto& plot = cache.plots().at(item_uid);

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
            item.name.c_str(), "Tick", "Items", expanded ? ImVec2(400, 200) : ImVec2(100, 50),
            (expanded ? 0 : ImPlotFlags_NoChild) | ImPlotFlags_CanvasOnly ^ ImPlotFlags_NoTitle,
            expanded ? 0 : ImPlotAxisFlags_NoDecorations, expanded ? 0 : ImPlotAxisFlags_NoLabel)) {
        auto plot_size = plot.container().size();

        // Shift the X axis one value to the left so that the total tick count equals the
        // last value plotted
        std::vector<int> plot_x(plot_size);
        for (std::size_t i = 1; i <= plot_size; i++) { plot_x[i - 1] = i; }

        ImPlot::PlotStairs(item.name.c_str(), plot_x.data(), plot.container().data(), plot_size);

        ImPlot::EndPlot();
    }
    ImPlot::PopStyleVar();
    ImPlot::PopStyleVar();
    ImPlot::PopStyleColor();
}

inline void draw_factory_inputs(const Factory& factory, const Factory::Cache& cache) {
    for (auto& input_uid : cache.inputs()) {
        const auto& item = factory.items.at(input_uid);
        imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar,
                                0xff + ((input_uid.value * 50) % 0xFF << 8) |
                                    ((input_uid.value * 186) % 0xFF << 16) |
                                    ((input_uid.value * 67) % 0xFF << 24));
        imnodes::BeginNode(input_uid.value);

        imnodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("Input");
        imnodes::EndNodeTitleBar();

        imnodes::BeginOutputAttribute(item.attribute_uid.value);
        ImGui::Text("%s", item.name.c_str());
        imnodes::EndOutputAttribute();

        imnodes::EndNode();

        imnodes::PopColorStyle();
    }
}

inline void draw_factory_machines(const Factory& factory,
                                  const Factory::Cache& _cache,
                                  Factory::MachinesT::const_iterator& out_machine_to_erase) {
    for (auto machine_it = factory.machines.cbegin(); machine_it != factory.machines.cend();
         machine_it++) {
        const auto machine_uid = machine_it->first;
        const auto& machine = machine_it->second;

        imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar,
                                0xff + ((machine_uid.value * 50) % 0xFF << 8) |
                                    ((machine_uid.value * 186) % 0xFF << 16) |
                                    ((machine_uid.value * 67) % 0xFF << 24));
        imnodes::BeginNode(machine_uid.value);

        imnodes::BeginNodeTitleBar();
        if (ImGui::CloseButton(ImGui::GetID("delete"),
                               ImVec2{ImGui::GetCursorPosX() + 3, ImGui::GetCursorPosY() + 45})) {
            out_machine_to_erase = machine_it;
        }
        ImGui::Dummy(ImVec2{10, 0});
        ImGui::SameLine();
        ImGui::TextUnformatted(machine.name.c_str());
        ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x);
        imnodes::EndNodeTitleBar();

        for (auto& input : machine.inputs) {
            const auto& item = factory.items.at(input.item);
            imnodes::BeginInputAttribute(input.uid.value);
            ImGui::Text("%i %s", input.quantity, item.name.c_str());
            imnodes::EndInputAttribute();
        }

        for (auto& output : machine.outputs) {
            const auto& item = factory.items.at(output.item);
            imnodes::BeginOutputAttribute(output.uid.value);
            ImGui::Indent(40);
            ImGui::Text("%i %s", output.quantity, item.name.c_str());
            imnodes::EndOutputAttribute();
        }

        ImGui::TextDisabled("%li t/op", machine.op_time.count());

        imnodes::EndNode();

        imnodes::PopColorStyle();
    }
}

inline void draw_factory_outputs(const Factory& factory, const Factory::Cache& cache) {
    for (auto& output : cache.outputs()) {
        const auto& item = factory.items.at(output);
        imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar,
                                0xff + ((output.value * 50) % 0xFF << 8) |
                                    ((output.value * 186) % 0xFF << 16) |
                                    ((output.value * 67) % 0xFF << 24));
        imnodes::BeginNode(output.value);

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

        imnodes::BeginInputAttribute(item.attribute_uid.value);
        draw_item_graph(factory, cache, output, expand_graph);
        imnodes::EndInputAttribute();

        imnodes::EndNode();

        imnodes::PopColorStyle();
    }
}

inline void
draw_factory_links(const Factory& factory, const Factory::Cache& cache, UidPool& uid_gen) {
    for (auto& [item_uid, node] : cache.item_nodes()) {
        for (auto& input : node.inputs) {
            for (auto& output : node.outputs) {
                // FIXME: These generated UIDs will be discarded in a single frame! We should reuse
                // them
                imnodes::Link(
                    uid_gen.generate().value,
                    factory.machines.at(input.machine).inputs[input.io_index].uid.value,
                    factory.machines.at(output.machine).outputs[output.io_index].uid.value);
            }
        }

        auto& item = factory.items.at(item_uid);

        if (item.type == Item::NodeType::Input) {
            for (auto& input : node.inputs) {
                imnodes::Link(uid_gen.generate().value,
                              factory.machines.at(input.machine).inputs[input.io_index].uid.value,
                              item.attribute_uid.value);
            }
        } else if (item.type == Item::NodeType::Output) {
            for (auto& output : node.outputs) {
                imnodes::Link(
                    uid_gen.generate().value, item.attribute_uid.value,
                    factory.machines.at(output.machine).outputs[output.io_index].uid.value);
            }
        }
    }
}

inline bool draw_machine_editor(const Factory& factory,
                                FactoryEditor::MachineEditor& editor,
                                UidPool& uid_pool,
                                std::optional<ImVec2> node_pos = std::nullopt) {

    imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar,
                            0xff + ((editor.machine_uid.value * 50) % 0xFF << 8) |
                                ((editor.machine_uid.value * 186) % 0xFF << 16) |
                                ((editor.machine_uid.value * 67) % 0xFF << 24));
    imnodes::BeginNode(editor.machine_uid.value);
    if (node_pos.has_value())
        imnodes::SetNodeGridSpacePos(editor.machine_uid.value, *node_pos);

    imnodes::BeginNodeTitleBar();
    ImGui::SelectableInput("##name", false, editor.machine.name.data(),
                           editor.machine.name.capacity());
    imnodes::EndNodeTitleBar();

    const auto& draw_io_manip = [&factory](ItemStream& obj) -> bool {
        ImGui::SetNextItemWidth(20);
        ImGui::DragInt("##o_quantity", &obj.quantity, 1.f, 1, 99);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(50);
        if (ImGui::BeginCombo("##o_item", factory.items.at(obj.item).name.c_str())) {
            for (auto& [item_uid, item] : factory.items) {
                const bool is_selected = obj.item == item_uid;
                if (ImGui::Selectable(item.name.c_str(), is_selected)) {
                    obj.item = item_uid;
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
    editor.machine.inputs.erase(
        std::remove_if(editor.machine.inputs.begin(), editor.machine.inputs.end(),
                       // FIXME: These generated UIDs will be discarded in a single frame! We should
                       // reuse them
                       [&draw_io_manip, &uid_pool](ItemStream& input) -> bool {
                           imnodes::BeginInputAttribute(uid_pool.generate().value);
                           bool remove = draw_io_manip(input);
                           imnodes::EndInputAttribute();
                           return remove;
                       }),
        editor.machine.inputs.end());
    if (ImGui::SmallButton("+##add_input")) {
        editor.machine.inputs.emplace_back(
            ItemStream{factory.items.begin()->first, 1, uid_pool.generate()});
    }

    // Draw outputs
    editor.machine.outputs.erase(
        std::remove_if(editor.machine.outputs.begin(), editor.machine.outputs.end(),
                       [&factory, &draw_io_manip, &uid_pool](ItemStream& output) -> bool {
                           imnodes::BeginOutputAttribute(uid_pool.generate().value);
                           ImGui::Indent(40);
                           bool remove = draw_io_manip(output);
                           ImGui::Unindent();
                           imnodes::EndOutputAttribute();
                           return remove;
                       }),
        editor.machine.outputs.end());

    ImGui::Indent(40);
    if (ImGui::SmallButton("+##add_output")) {
        editor.machine.outputs.emplace_back(
            ItemStream{factory.items.begin()->first, 1, uid_pool.generate()});
    }
    ImGui::Unindent();

    ImGui::SetNextItemWidth(50);
    int op_time = editor.machine.op_time.count();
    ImGui::DragInt("##tpop", &op_time, 1.f, 1, 99999);
    editor.machine.op_time = util::ticks(op_time);
    ImGui::SameLine();
    ImGui::TextDisabled("t/op");

    bool is_finished = ImGui::Button("Finish");

    imnodes::EndNode();

    imnodes::PopColorStyle();

    return is_finished;
}

} // namespace fmk