#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_node_editor.h>
#include <implot.h>

#include "factory.hpp"
#include "util/more_imgui.hpp"

namespace fmk {

namespace ed = ax::NodeEditor;

inline void draw_item_graph(const Factory& factory,
                            const Factory::Cache& cache,
                            const Uid item_uid,
                            bool expanded = true,
                            bool reload_plot_limits = false) {
    auto& item = factory.items.at(item_uid);
    auto& plot = cache.plots().at(item_uid);

    ImPlot::SetNextPlotLimits(
        1, static_cast<double>(cache.ticks_simulated()) + 1., 0., plot.max_value() + 1,
        (expanded && !reload_plot_limits) ? ImGuiCond_Appearing : ImGuiCond_Always);

    {
        double ticks_x[] = {1, static_cast<double>(cache.ticks_simulated()) / 2,
                            static_cast<double>(cache.ticks_simulated())};
        ImPlot::SetNextPlotTicksX(ticks_x, 3);
    }
    if (!expanded) {
        double ticks_y[] = {0, static_cast<double>(plot.max_value())};
        ImPlot::SetNextPlotTicksY(ticks_y, 2);
    }

    ImPlot::PushStyleColor(ImPlotCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.5f);
    ImPlot::PushStyleVar(ImPlotStyleVar_LabelPadding, ImVec2(0.75f, 1));
    ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(expanded ? 10 : 0, 5));
    if (ImPlot::BeginPlot(item.name.c_str(), "Tick", "Items",
                          expanded ? ImVec2(400, 200) : ImVec2(100, 50),
                          (expanded ? 0 : ImPlotFlags_NoChild) |
                              ImPlotFlags_CanvasOnly ^ (expanded ? ImPlotFlags_NoTitle : 0) |
                              ImPlotFlags_AntiAliased,
                          expanded ? 0 : ImPlotAxisFlags_NoDecorations,
                          (expanded ? 0 : ImPlotAxisFlags_NoLabel) | ImPlotAxisFlags_AutoFit)) {
        auto plot_size = plot.container().size();

        // Shift the X axis one value to the left so that the total tick count equals the
        // last value plotted
        std::vector<int> plot_x(plot_size);
        for (int i = 1; i <= plot_size; i++) { plot_x[i - 1] = i; }

        ImPlot::PlotShaded(item.name.c_str(), plot_x.data(), plot.container().data(),
                           static_cast<int>(plot_size));
        ImPlot::PlotStairs(item.name.c_str(), plot_x.data(), plot.container().data(),
                           static_cast<int>(plot_size));

        ImPlot::EndPlot();
    }
    ImPlot::PopStyleVar();
    ImPlot::PopStyleVar();
    ImPlot::PopStyleColor();
}

bool delete_button(Uid id) {
    bool result = ImGui::CloseButton(ImGui::GetID(fmt::format("delete##{}", id.value).c_str()),
                                     ImGui::GetCursorPos());

    ImGui::Dummy(ImVec2{15, 0});

    return result;
}

void draw_pin() {
    const auto line_height = ImGui::GetTextLineHeight();
    ImVec2 size{line_height, line_height};

    auto rect = ImRect(ImGui::GetCursorScreenPos(), ImGui::GetCursorScreenPos() + ImVec2(20., 20.));
    auto rect_w = rect.Max.x - rect.Min.x;
    auto rect_center_x = (rect.Min.x + rect.Max.x) * 0.5f;
    auto rect_center_y = (rect.Min.y + rect.Max.y) * 0.5f;
    auto rect_center = ImVec2(rect_center_x, rect_center_y);
    const auto r = 0.607f * rect_w / 2.0f;
    const auto c = rect_center;
    auto drawList = ImGui::GetWindowDrawList();

    drawList->PathLineTo(c + ImVec2(0, -r));
    drawList->PathLineTo(c + ImVec2(r, 0));
    drawList->PathLineTo(c + ImVec2(0, r));
    drawList->PathLineTo(c + ImVec2(-r, 0));
    drawList->PathFillConvex(0xFFFFFFFF);
    ImGui::Dummy(size);
}

/// Returns the input to delete, if any
inline std::optional<Uid> draw_factory_inputs(const Factory& factory, const Factory::Cache& cache) {
    std::optional<Uid> to_delete;

    for (auto& input_uid : cache.inputs()) {
        const auto& item = factory.items.at(input_uid);
        /*
        imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar,
                                0xff + ((input_uid.value * 50) % 0xFF << 8) |
                                    ((input_uid.value * 186) % 0xFF << 16) |
                                    ((input_uid.value * 67) % 0xFF << 24));
                                    */
        ed::BeginNode(input_uid.value);

        if (!cache.item_nodes().contains(input_uid) ||
            cache.item_nodes().at(input_uid).inputs.empty()) {
            if (delete_button(input_uid)) {
                to_delete = input_uid;
            }
            ImGui::SameLine();
        }
        ImGui::TextUnformatted("Input");

        ed::BeginPin(item.attribute_uid.value, ed::PinKind::Output);
        ImGui::Text("%s", item.name.c_str());
        ImGui::SameLine();
        draw_pin();
        ed::PinRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        ed::EndPin();

        ed::EndNode();
    }

    return to_delete;
}

inline void draw_factory_machines(const Factory& factory,
                                  const Factory::Cache& _cache,
                                  Factory::MachinesT::const_iterator& out_machine_to_erase,
                                  Factory::MachinesT::const_iterator& out_machine_to_edit) {
    for (auto machine_it = factory.machines.cbegin(); machine_it != factory.machines.cend();
         machine_it++) {
        const auto machine_uid = machine_it->first;
        const auto& machine = machine_it->second;

        /*
        imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar,
                                0xff + ((machine_uid.value * 50) % 0xFF << 8) |
                                    ((machine_uid.value * 186) % 0xFF << 16) |
                                    ((machine_uid.value * 67) % 0xFF << 24)); */
        ed::BeginNode(machine_uid.value);

        if (delete_button(machine_uid)) {
            out_machine_to_erase = machine_it;
        }
        ImGui::SameLine();
        if (ImGui::Button(fmt::format("Edit##{}", machine_uid.value).c_str())) {
            out_machine_to_edit = machine_it;
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(machine.name.c_str());

        for (auto& input : machine.inputs) {
            const auto& item = factory.items.at(input.item);
            ed::BeginPin(input.uid.value, ed::PinKind::Input);
            {
                draw_pin();
                ed::PinRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
                ImGui::SameLine();
                ImGui::Text("%i %s", input.quantity, item.name.c_str());
            }
            ed::EndPin();
        }

        for (auto& output : machine.outputs) {
            const auto& item = factory.items.at(output.item);
            ed::BeginPin(output.uid.value, ed::PinKind::Output);
            {
                ImGui::Text("%i %s", output.quantity, item.name.c_str());
                ImGui::SameLine();
                draw_pin();
                ed::PinRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
            }
            ed::EndPin();
        }

        ImGui::TextDisabled("%li t/op", machine.op_time.count());

        ed::EndNode();
    }
}

/// Returns the input to delete, if any
inline std::optional<Uid> draw_factory_outputs(const Factory& factory,
                                               const Factory::Cache& cache) {
    std::optional<Uid> to_delete;

    for (auto& output_uid : cache.outputs()) {
        const auto& item = factory.items.at(output_uid);
        /*
        imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar,
                                0xff + ((output_uid.value * 50) % 0xFF << 8) |
                                    ((output_uid.value * 186) % 0xFF << 16) |
                                    ((output_uid.value * 67) % 0xFF << 24)); */
        ed::BeginNode(output_uid.value);

        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
        if (!cache.item_nodes().contains(output_uid) ||
            cache.item_nodes().at(output_uid).outputs.empty()) {
            if (delete_button(output_uid)) {
                to_delete = output_uid;
            }
            ImGui::SameLine();
        }
        ImGui::Text("Output");
        ImGui::PopStyleColor(2);

        ed::BeginPin(item.attribute_uid.value, ed::PinKind::Input);
        draw_pin();
        ed::PinRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        ImGui::SameLine();
        ImGui::Text(item.name.c_str());
        draw_item_graph(factory, cache, output_uid, false);
        ed::EndPin();

        ed::EndNode();
    }

    return to_delete;
}

inline void
draw_factory_links(const Factory& factory, const Factory::Cache& cache, UidPool& uid_gen) {
    static std::vector<Uid> link_uids;
    std::size_t next_uid_idx = 0;

    auto get_uid = [&next_uid_idx, &uid_gen]() -> Uid {
        if (next_uid_idx >= link_uids.size()) {
            const auto uid = uid_gen.generate();
            link_uids.push_back(uid);
            next_uid_idx++;
            return uid;
        } else {
            return link_uids[next_uid_idx++];
        }
    };
    for (auto& [item_uid, node] : cache.item_nodes()) {
        for (auto& input : node.inputs) {
            for (auto& output : node.outputs) {
                ed::Link(get_uid().value,
                         factory.machines.at(input.machine).inputs[input.io_index].uid.value,
                         factory.machines.at(output.machine).outputs[output.io_index].uid.value);
            }
        }

        auto& item = factory.items.at(item_uid);

        if (item.type == Item::NodeType::Input) {
            for (auto& input : node.inputs) {
                ed::Link(get_uid().value,
                         factory.machines.at(input.machine).inputs[input.io_index].uid.value,
                         item.attribute_uid.value);
            }
        } else if (item.type == Item::NodeType::Output) {
            for (auto& output : node.outputs) {
                ed::Link(get_uid().value, item.attribute_uid.value,
                         factory.machines.at(output.machine).outputs[output.io_index].uid.value);
            }
        }
    }
}

inline bool draw_machine_editor(const Factory& factory,
                                FactoryEditor::MachineEditor& editor,
                                UidPool& uid_pool,
                                std::optional<ImVec2> node_pos = std::nullopt) {
    /*
        imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar,
                                0xff + ((editor.machine_uid.value * 50) % 0xFF << 8) |
                                    ((editor.machine_uid.value * 186) % 0xFF << 16) |
                                    ((editor.machine_uid.value * 67) % 0xFF << 24));*/
    ed::BeginNode(editor.machine_uid.value);
    if (node_pos.has_value())
        ed::SetNodePosition(editor.machine_uid.value, *node_pos);

    ImGui::SelectableInput("##name", false, &editor.machine.name);

    const auto& draw_io_manip = [&factory](ItemStream& obj) -> bool {
        ImGui::SetNextItemWidth(40);
        ImGui::DragInt("##quantity", &obj.quantity, 1.f, 1, 99999);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        if (ImGui::BeginCombo("##item", factory.items.at(obj.item).name.c_str())) {
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
    ImGui::TextDisabled("Inputs");
    editor.machine.inputs.erase(
        std::remove_if(editor.machine.inputs.begin(), editor.machine.inputs.end(),
                       [&draw_io_manip, &uid_pool](ItemStream& input) -> bool {
                           ed::BeginPin(input.uid.value, ed::PinKind::Input);
                           ImGui::PushID(input.uid.value);
                           bool remove = draw_io_manip(input);
                           ImGui::PopID();
                           ed::EndPin();
                           return remove;
                       }),
        editor.machine.inputs.end());
    if (ImGui::SmallButton("+##add_input")) {
        editor.machine.inputs.emplace_back(
            ItemStream{factory.items.begin()->first, 1, uid_pool.generate()});
    }

    // Draw outputs
    ImGui::Indent(50);
    ImGui::TextDisabled("Outputs");
    editor.machine.outputs.erase(
        std::remove_if(editor.machine.outputs.begin(), editor.machine.outputs.end(),
                       [&draw_io_manip, &uid_pool](ItemStream& output) -> bool {
                           ed::BeginPin(output.uid.value, ed::PinKind::Output);
                           ImGui::PushID(output.uid.value);
                           bool remove = draw_io_manip(output);
                           ImGui::PopID();
                           ed::EndPin();
                           return remove;
                       }),
        editor.machine.outputs.end());

    if (ImGui::SmallButton("+##add_output")) {
        editor.machine.outputs.emplace_back(
            ItemStream{factory.items.begin()->first, 1, uid_pool.generate()});
    }
    ImGui::Unindent();

    ImGui::SetNextItemWidth(50);
    int op_time = static_cast<int>(editor.machine.op_time.count());
    ImGui::DragInt("##tpop", &op_time, 1.f, 1, 99999);
    editor.machine.op_time = util::ticks(op_time);
    ImGui::SameLine();
    ImGui::TextDisabled("t/op");

    bool is_finished = ImGui::Button("Finish");

    ed::EndNode();

    return is_finished;
}

} // namespace fmk