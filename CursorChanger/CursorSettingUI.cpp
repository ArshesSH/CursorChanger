#include "CursorSettingUI.h"

#include "Debugger.h"
#include "FileSelector.h"
#include "imgui.h"

CursorSettingUI::~CursorSettingUI()
{
}

void CursorSettingUI::UpdateImGui()
{
    ImGui::Dummy(spaceSize);
    ImGui::SeparatorText("Cursor Setting");
    if (ImGui::Button("Select cursor path"))
    {
        pCursorSetting->cursorPath = FileSelector::OpenFileSelectDialog("Cursor Files", {"*.cur", "*.ani"});
    }
    ImGui::Text("Cursor path: %s", pCursorSetting->cursorPath.c_str());

    
    ImGui::Dummy(spaceSize);
    ImGui::SeparatorText("Auto change");
    if (ImGui::Checkbox("Auto change cursor by process", &pCursorSetting->shouldChangeByProcess))
    {
        if (pCursorSetting->shouldChangeByProcess == false)
        {
            onClickRestoreCursor();
        }
    }
    if (pCursorSetting->shouldChangeByProcess)
    {
        if (ImGui::Checkbox("Focus only", &pCursorSetting->isFocusOnly))
        {
            if (onClickFocusOnly != nullptr)
            {
                onClickFocusOnly();
            }
        }
        ImGui::SetItemTooltip("Change cursor only when the target process is in focus");
        ImGui::Dummy(spaceSize);

        ImGui::Text("Process Name:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(300.0f);
        if (ImGui::InputText("##ProcessNameInput", processNameBuffer, sizeof(processNameBuffer)))
        {
            pCursorSetting->targetProcess = processNameBuffer;
        }
        ImGui::Text("Target process: %s", pCursorSetting->targetProcess.c_str());
        ImGui::SameLine();
    }
    
    ImGui::Dummy(spaceSize);
    if (ImGui::Button("Change Cursor Manually"))
    {
        if (onClickChangeCursor != nullptr)
        {
            onClickChangeCursor();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Restore Cursor"))
    {
        if (onClickRestoreCursor != nullptr)
        {
            onClickRestoreCursor();
        }
    }
    ImGui::Dummy(spaceSize);
}
