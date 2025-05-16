#include "CursorSettingUI.h"

#include "FileSelector.h"
#include "imgui.h"
#include "SelectorUI.h"

CursorSettingUI::~CursorSettingUI()
{
}

void CursorSettingUI::UpdateImGui()
{
    ImGui::SeparatorText("Cursor Setting");
    if (ImGui::Button("Select cursor path"))
    {
        pCursorSetting->cursorPath = FileSelector::OpenFileSelectDialog("Cursor Files", {"*.cur", "*.ani"});
    }
    ImGui::Text("Cursor path: %s", pCursorSetting->cursorPath.c_str());

    
    ImGui::Dummy(spaceSize);
    ImGui::SeparatorText("Auto change");
    ImGui::Checkbox("Auto change cursor by process", &pCursorSetting->shouldChangeByProcess);

    if (pCursorSetting->shouldChangeByProcess)
    {
        ImGui::Text("Process Name:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(300.0f);
        if (ImGui::InputText("##ProcessNameInput", processNameBuffer, sizeof(processNameBuffer)))
        {
            pCursorSetting->targetProcess = processNameBuffer;
        }
        ImGui::Text("Target process: %s", pCursorSetting->targetProcess.c_str());
    }
    
    ImGui::Dummy(spaceSize);
    ImGui::Separator();
    if (ImGui::Button("Save") )
    {
        if (onClickSaveSetting != nullptr)
        {
            onClickSaveSetting();
        }
    }
    ImGui::SameLine();
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
