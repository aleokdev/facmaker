#include <imgui.h>
#include <imnodes.h>

#include "editor/factory_editor.hpp"

namespace fmk {

FactoryEditor::FactoryEditor() { imnodes_ctx = imnodes::EditorContextCreate(); }

void FactoryEditor::draw() {
    imnodes::EditorContextSet(imnodes_ctx);

    ImGui::SetNextWindowSize(ImVec2{500, 500}, ImGuiCond_Appearing);
    ImGui::Begin("simple node editor");

    imnodes::BeginNodeEditor();
    imnodes::BeginNode(1);

    imnodes::BeginNodeTitleBar();
    ImGui::TextUnformatted("simple node :)");
    imnodes::EndNodeTitleBar();

    imnodes::BeginInputAttribute(2);
    ImGui::Text("input");
    imnodes::EndInputAttribute();

    imnodes::BeginOutputAttribute(3);
    ImGui::Indent(40);
    ImGui::Text("output");
    imnodes::EndOutputAttribute();

    imnodes::EndNode();
    imnodes::EndNodeEditor();

    ImGui::End();
}

} // namespace fmk