#include <imgui.h>

namespace ImGui {

bool SelectableInput(const char* str_id,
                     bool selected,
                     char* buf,
                     size_t buf_size,
                     ImGuiSelectableFlags flags = ImGuiSelectableFlags_None);

} // namespace ImGui