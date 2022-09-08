#include "util/more_imgui.hpp"

#include <cstring>
#include <imgui_internal.h>

namespace ImGui {

bool SelectableInput(
    const char* str_id, bool selected, char* buf, size_t buf_size, ImGuiSelectableFlags flags) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImVec2 pos_before = window->DC.CursorPos;

    PushID(str_id);
    PushStyleVar(ImGuiStyleVar_ItemSpacing,
                 ImVec2(g.Style.ItemSpacing.x, g.Style.FramePadding.y * 2.0f));
    bool ret = Selectable("##Selectable", selected,
                          flags | ImGuiSelectableFlags_AllowDoubleClick |
                              ImGuiSelectableFlags_AllowItemOverlap);
    PopStyleVar();

    ImGuiID id = window->GetID("##Input");
    bool temp_input_is_active = TempInputIsActive(id);
    bool temp_input_start = ret && IsMouseDoubleClicked(0);

    if (temp_input_start)
        SetActiveID(id, window);

    if (temp_input_is_active || temp_input_start) {
        ImVec2 pos_after = window->DC.CursorPos;
        window->DC.CursorPos = pos_before;
        ret = TempInputText(window->DC.LastItemRect, id, "##Input", buf, (int)buf_size,
                            ImGuiInputTextFlags_None);
        window->DC.CursorPos = pos_after;
    } else {
        window->DrawList->AddText(pos_before, GetColorU32(ImGuiCol_Text), buf);
    }

    PopID();
    return ret;
}

bool SelectableInput(const char* str_id,
                     bool selected,
                     std::string* str,
                     ImGuiSelectableFlags flags) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImVec2 pos_before = window->DC.CursorPos;

    PushID(str_id);
    PushStyleVar(ImGuiStyleVar_ItemSpacing,
                 ImVec2(g.Style.ItemSpacing.x, g.Style.FramePadding.y * 2.0f));
    bool ret = Selectable("##Selectable", selected,
                          flags | ImGuiSelectableFlags_AllowDoubleClick |
                              ImGuiSelectableFlags_AllowItemOverlap);
    PopStyleVar();

    ImGuiID id = window->GetID("##Input");
    bool temp_input_is_active = TempInputIsActive(id);
    bool temp_input_start = ret && IsMouseDoubleClicked(0);

    if (temp_input_start)
        SetActiveID(id, window);

    if (temp_input_is_active || temp_input_start) {
        ImVec2 pos_after = window->DC.CursorPos;
        window->DC.CursorPos = pos_before;
        ret = TempInputText(window->DC.LastItemRect, id, "##Input", str->data(), str->capacity(),
                            ImGuiInputTextFlags_None);
        str->resize(std::strlen(str->c_str()));
        window->DC.CursorPos = pos_after;
    } else {
        window->DrawList->AddText(pos_before, GetColorU32(ImGuiCol_Text), str->c_str());
    }

    PopID();
    return ret;
}

struct InputTextCallback_UserData {
    std::string* Str;
    ImGuiInputTextCallback ChainCallback;
    void* ChainCallbackUserData;
};

static int InputTextCallback(ImGuiInputTextCallbackData* data) {
    InputTextCallback_UserData* user_data = (InputTextCallback_UserData*)data->UserData;
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        // Resize string callback
        // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we
        // need to set them back to what we want.
        std::string* str = user_data->Str;
        IM_ASSERT(data->Buf == str->c_str());
        str->resize(data->BufTextLen);
        data->Buf = (char*)str->c_str();
    } else if (user_data->ChainCallback) {
        // Forward to user callback, if any
        data->UserData = user_data->ChainCallbackUserData;
        return user_data->ChainCallback(data);
    }
    return 0;
}

bool InputText(const char* label,
               std::string* str,
               ImGuiInputTextFlags flags,
               ImGuiInputTextCallback callback,
               void* user_data) {
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return InputText(label, str->data(), str->capacity(), flags, InputTextCallback, &cb_user_data);
}

bool InputTextMultiline(const char* label,
                        std::string* str,
                        const ImVec2& size,
                        ImGuiInputTextFlags flags,
                        ImGuiInputTextCallback callback,
                        void* user_data) {
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return InputTextMultiline(label, str->data(), str->capacity(), size, flags, InputTextCallback,
                              &cb_user_data);
}

bool InputTextWithHint(const char* label,
                       const char* hint,
                       std::string* str,
                       ImGuiInputTextFlags flags,
                       ImGuiInputTextCallback callback,
                       void* user_data) {
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return InputTextWithHint(label, hint, str->data(), str->capacity(), flags, InputTextCallback,
                             &cb_user_data);
}

} // namespace ImGui