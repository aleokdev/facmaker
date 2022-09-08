#pragma once

#include <imgui.h>
#include <string>

namespace ImGui {

bool SelectableInput(const char* str_id,
                     bool selected,
                     char* buf,
                     size_t buf_size,
                     ImGuiSelectableFlags flags = ImGuiSelectableFlags_None);

bool SelectableInput(const char* str_id,
                     bool selected,
                     std::string* str,
                     ImGuiSelectableFlags flags = ImGuiSelectableFlags_None);

// From imgui/misc/cpp/imgui_stdlib:

// ImGui::InputText() with std::string
// Because text input needs dynamic resizing, we need to setup a callback to grow the capacity
IMGUI_API bool InputText(const char* label,
                         std::string* str,
                         ImGuiInputTextFlags flags = 0,
                         ImGuiInputTextCallback callback = NULL,
                         void* user_data = NULL);
IMGUI_API bool InputTextMultiline(const char* label,
                                  std::string* str,
                                  const ImVec2& size = ImVec2(0, 0),
                                  ImGuiInputTextFlags flags = 0,
                                  ImGuiInputTextCallback callback = NULL,
                                  void* user_data = NULL);
IMGUI_API bool InputTextWithHint(const char* label,
                                 const char* hint,
                                 std::string* str,
                                 ImGuiInputTextFlags flags = 0,
                                 ImGuiInputTextCallback callback = NULL,
                                 void* user_data = NULL);

} // namespace ImGui