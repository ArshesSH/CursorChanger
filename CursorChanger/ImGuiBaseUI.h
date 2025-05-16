#pragma once

class ImGuiBaseUI
{
public:
    ImGuiBaseUI() = default;
    virtual ~ImGuiBaseUI() = default;
    virtual void UpdateImGui() = 0;
};
