#include "editor/factory_editor.hpp"

#include <boost/json.hpp>
#include <charconv>
#include <fmt/core.h>
#include <fstream>
#include <imgui.h>
#include <imnodes.h>
#include <implot.h>
#include <iomanip>
#include <iostream>
#include <optional>
#include <plog/Log.h>
#include <string>
#include <string_view>

#include "editor/draw_helpers.hpp"
#include "pfd/pfd.hpp"

namespace json = boost::json;

namespace fmk {

FactoryEditor::FactoryEditor() :
    factory{.items = {}, .machines = {}}, uid_pool(Uid(Uid::INVALID_VALUE + 1)) {
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
    ImGui::Begin("Factory Display", nullptr, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open...")) {
                const auto selection = pfd::open_file("Open Factory").result();
                if (!selection.empty()) {
                    std::ifstream file(selection[0]);
                    parse_factory_json(file);
                    PLOGD << "Imported data from '" << selection[0] << "'";
                }
            }
            if (ImGui::MenuItem("Save As...")) {
                const auto destination = pfd::save_file("Save Factory").result();
                if (!destination.empty()) {
                    std::ofstream file(destination);
                    output_factory_json(file);
                    PLOGD << "Exported data to '" << destination << "'";
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
    auto machine_to_erase = factory.machines.cend();

    if (const auto input_to_delete = draw_factory_inputs(factory, cache.factory_cache)) {
        factory.items.erase(*input_to_delete);
        regenerate_cache();
    }
    draw_factory_machines(factory, cache.factory_cache, machine_to_erase);
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
    }

    imnodes::EndNodeEditor();

    if (int start_attr; imnodes::IsLinkDropped(&start_attr)) {
        [&]() {
            for (const auto& [machine_uid, machine] : factory.machines) {
                for (const auto& input : machine.inputs) {
                    if (start_attr == input.uid.value) {
                        // Convert this machine's input item into an input!
                        factory.items.at(input.item).type = Item::NodeType::Input;

                        regenerate_cache();
                        return;
                    }
                }
            }

            for (const auto& [machine_uid, machine] : factory.machines) {
                for (const auto& output : machine.outputs) {
                    if (start_attr == output.uid.value) {
                        // Convert this machine's output item into an output!
                        factory.items.at(output.item).type = Item::NodeType::Output;

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
            new_machine.emplace(
                MachineEditor{Machine{std::move(new_machine_name)}, uid_pool.generate()});

            editor_node_start_pos = ImVec2{
                ImGui::GetMousePos().x - editor_pos.x - imnodes::EditorContextGetPanning().x,
                ImGui::GetMousePos().y - editor_pos.y - imnodes::EditorContextGetPanning().y};
        }
        ImGui::EndPopup();
    }

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
                ImGui::Text(item.name.data());
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
                ImGui::InputText("Name", item_edit_name.data(), item_edit_name.capacity());
                ImGui::Combo("Type", reinterpret_cast<int*>(&item_edit_type),
                             "Input\0Output\0Internal");
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
            factory.items[uid_pool.generate()] = Item{
                static_cast<Item::NodeType>(type),
                starting_quantity,
                name,
            };
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

    // We set the editor context to be able to set positions
    imnodes::EditorContextSet(imnodes_ctx);

    auto parse_xy = [](json::object const& object, Uid uid) {
        if (auto x_val = object.if_contains("x")) {
            imnodes::SetNodeGridSpacePos(uid.value,
                                         {static_cast<float>(x_val->as_double()),
                                          static_cast<float>(object.at("y").as_double())});
        }
    };

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
        if (const auto obj = val.if_object()) {
            Uid next_uid(-1);
            if (const auto uid_pool_val = obj->if_contains("uid_pool")) {
                next_uid.value =
                    static_cast<int>(uid_pool_val->as_object().at("next_uid").as_int64());
            } else {
                PLOG_ERROR << "JSON loading error: `uid_pool` key not found";
                had_errors = true;
            }
            UidPool parse_uid_pool(next_uid);
            if (const auto items_val = obj->if_contains("items")) {
                if (const auto items = items_val->if_object()) {
                    for (const auto& [item_uid_str, item_val] : *items) {
                        Uid item_uid(Uid::INVALID_VALUE);
                        const auto [_, ec] = std::from_chars(
                            item_uid_str.data(), item_uid_str.data() + item_uid_str.size(),
                            item_uid.value);

                        if (ec != std::errc()) {
                            PLOG_ERROR << "JSON loading error: Could not parse UID";
                            had_errors = true;
                        }

                        if (auto item = item_val.if_object()) {
                            parsed_items[item_uid].name = item->at("name").as_string();
                            if (auto ty = item->at("type").if_string()) {
                                if (*ty == "input") {
                                    parsed_items[item_uid].type = Item::NodeType::Input;
                                    parsed_items[item_uid].attribute_uid = uid_pool.generate();
                                } else if (*ty == "output") {
                                    parsed_items[item_uid].type = Item::NodeType::Output;
                                    parsed_items[item_uid].attribute_uid = uid_pool.generate();
                                } else if (*ty == "internal") {
                                    parsed_items[item_uid].type = Item::NodeType::Internal;
                                }
                            }
                            if (auto quantity = item->at("start_with").if_int64()) {
                                parsed_items[item_uid].starting_quantity =
                                    static_cast<int>(*quantity);
                            }

                            parse_xy(*item, item_uid);
                        }
                    }
                } else {
                    PLOG_ERROR << "JSON loading error: `items` must be an object`";
                    had_errors = true;
                }
            }

            if (auto machines_val = obj->if_contains("machines")) {
                if (const auto machines = machines_val->if_object()) {
                    for (const auto& [machine_uid_str, machine_val] : *machines) {
                        Uid machine_uid(Uid::INVALID_VALUE);
                        const auto [_, ec] = std::from_chars(
                            machine_uid_str.data(), machine_uid_str.data() + machine_uid_str.size(),
                            machine_uid.value);

                        if (ec != std::errc()) {
                            PLOG_ERROR << "JSON loading error: Could not parse UID";
                            had_errors = true;
                        }

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
                                    for (const auto& [input_uid_str, input_qty] : *inputs) {
                                        Uid input_uid(-1);
                                        const auto [_, ec] = std::from_chars(
                                            input_uid_str.data(),
                                            input_uid_str.data() + input_uid_str.size(),
                                            input_uid.value);

                                        if (ec != std::errc()) {
                                            PLOG_ERROR << "JSON loading error: Could not parse UID";
                                            had_errors = true;
                                        }
                                        if (auto quantity = input_qty.if_int64()) {
                                            parsed_items.insert({input_uid, Item{}});
                                            result.inputs.emplace_back(
                                                ItemStream{input_uid, static_cast<int>(*quantity),
                                                           parse_uid_pool.generate()});
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
                                    for (const auto& [output_uid_str, output_qty] : *outputs) {
                                        Uid output_uid(-1);
                                        const auto [_, ec] = std::from_chars(
                                            output_uid_str.data(),
                                            output_uid_str.data() + output_uid_str.size(),
                                            output_uid.value);

                                        if (ec != std::errc()) {
                                            PLOG_ERROR << "JSON loading error: Could not parse UID";
                                            had_errors = true;
                                        }
                                        if (auto quantity = output_qty.if_int64()) {
                                            parsed_items.insert({output_uid, Item{}});
                                            result.outputs.emplace_back(
                                                ItemStream{output_uid, static_cast<int>(*quantity),
                                                           parse_uid_pool.generate()});
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

                            parse_xy(*machine, machine_uid);

                            parsed_machines[machine_uid] = result;
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

void FactoryEditor::output_factory_json(std::ostream& out) const {
    out << "{";

    // Items
    {
        out << "\"items\":{";
        for (auto item_it = factory.items.cbegin(); item_it != factory.items.cend(); item_it++) {
            const auto& [item_uid, item] = *item_it;
            out << "\"" << item_uid.value << "\":{\"name\":\"" << item.name << "\",\"type\":\"";
            bool write_xy = false;
            switch (item.type) {
                case Item::NodeType::Input:
                    out << "input";
                    write_xy = true;
                    break;
                case Item::NodeType::Output:
                    out << "output";
                    write_xy = true;
                    break;
                case Item::NodeType::Internal: out << "internal"; break;
            }
            out << "\",\"start_with\":" << item.starting_quantity;
            if (write_xy) {
                imnodes::EditorContextSet(imnodes_ctx);
                const auto [x, y] = imnodes::GetNodeGridSpacePos(item_uid.value);
                out << ",\"x\":" << std::fixed << std::setprecision(1) << x << ",\"y\":" << y
                    << "}";
            } else {
                out << "}";
            }
            if (std::next(item_it) != factory.items.cend()) {
                out << ",";
            }
        }
        out << "},";
    }

    // Machines
    {
        out << "\"machines\":{";
        const auto& machines = factory.machines;
        for (auto machine_it = machines.cbegin(); machine_it != machines.cend(); machine_it++) {
            const auto& [machine_uid, machine] = *machine_it;

            out << "\"" << machine_uid.value << "\":"
                << "{";
            {
                out << "\"name\":\"" << machine.name << "\",";
                out << "\"inputs\":{";
                {
                    const auto& inputs = machine.inputs;
                    for (std::size_t i = 0; i < inputs.size(); i++) {
                        out << "\"" << inputs[i].item.value << "\":" << inputs[i].quantity;
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
                        out << "\"" << outputs[i].item.value << "\":" << outputs[i].quantity;
                        if (i < outputs.size() - 1) {
                            out << ",";
                        }
                    }
                }
                out << "},";
                out << "\"time\":" << machine.op_time.count() << ",";
                const auto [x, y] = imnodes::GetNodeGridSpacePos(machine_uid.value);
                out << "\"x\":" << std::fixed << std::setprecision(1) << x << ",\"y\":" << y;
            }
            out << "}";
            if (std::next(machine_it) != factory.machines.cend()) {
                out << ",";
            }
        }
        out << "},";
    }

    // Simulate value
    { out << "\"simulate\":" << cache.factory_cache.ticks_simulated() << ","; }

    { out << "\"uid_pool\":{\"next_uid\":" << uid_pool.get_next_uid().value << "}"; }

    out << "}";
}

} // namespace fmk