#pragma once
#include <functional>
#include <memory>

#include "CursorSetting.h"

class CursorSettingUI
{
public:
    CursorSettingUI(const std::shared_ptr<CursorSetting>& pCursorSetting, 
                    const std::function<void()>& onClickSaveSetting = nullptr,
                    const std::function<void()>& onClickChangeCursor = nullptr)
        : pCursorSetting(pCursorSetting),
          OnClickSaveSetting(onClickSaveSetting),
          OnClickChangeCursor(onClickChangeCursor)
    {}
    ~CursorSettingUI();
    
    void UpdateImGui();

private:
    std::shared_ptr<CursorSetting> pCursorSetting;
    
    // Interface
    std::function<void()> OnClickSaveSetting;
    std::function<void()> OnClickChangeCursor;
};
