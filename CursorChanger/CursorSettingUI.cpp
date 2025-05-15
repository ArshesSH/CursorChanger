#include "CursorSettingUI.h"

#include "FileSelector.h"
#include "imgui.h"

CursorSettingUI::~CursorSettingUI()
{
}

void CursorSettingUI::UpdateImGui()
{
    ImGui::Text("[Cursor setting]");
    if (ImGui::Button("Select cursor path"))
    {
        pCursorSetting->cursorPath = FileSelector::OpenFileSelectDialog("Cursor Files", {"*.cur", "*.ani"});
    }
    ImGui::Text("Cursor path: %s", pCursorSetting->cursorPath.c_str());
    ImGui::Text("Target process: %s", pCursorSetting->targetProcess.c_str());
    if (ImGui::Button("Save") )
    {
        if (onClickSaveSetting != nullptr)
        {
            onClickSaveSetting();
        }
    }
    ImGui::Separator();
    if (ImGui::Button("Change Cursor"))
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
}
