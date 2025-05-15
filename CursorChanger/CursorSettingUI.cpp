#include "CursorSettingUI.h"

#include "FileSelector.h"
#include "imgui.h"

CursorSettingUI::~CursorSettingUI()
{
}

void CursorSettingUI::UpdateImGui()
{
    ImGui::Text("[Cursor setting]");
    if (ImGui::Button("Change cursor"))
    {
        pCursorSetting->cursorPath = FileSelector::OpenFileSelectDialog("Cursor Files", {"*.cur", "*.ani"});
    }
    ImGui::Text("Cursor path: %s", pCursorSetting->cursorPath.c_str());
    ImGui::Text("Target process: %s", pCursorSetting->targetProcess.c_str());
    if (ImGui::Button("Save") )
    {
        if (OnClickSaveSetting != nullptr)
        {
            OnClickSaveSetting();
        }
    }
    ImGui::Separator();
    if (ImGui::Button("Change Cursor"))
    {
        if (OnClickChangeCursor != nullptr)
        {
            OnClickChangeCursor();
        }
    }
}
