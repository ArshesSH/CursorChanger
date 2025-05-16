#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <functional>

#include "imgui.h"
#include "ImGuiBaseUI.h"

template <typename T>
class SelectorUI: public ImGuiBaseUI
{
public:
    struct Item
    {
        std::wstring name;
        T value;
    };

public:
    SelectorUI(const std::vector<Item>& itemList, bool isDialog = false)
        : itemList(itemList),
          filteredItemList(itemList), // 초기화 필요
          isDialog(isDialog),
          selectedIndex(-1),
          isSelectionConfirmed(false),
          isOpen(true)
    {
        if (!filteredItemList.empty())
            selectedIndex = 0;
    }
    ~SelectorUI() = default;

    void UpdateImGui() override;
    void UpdateFilteredItems(const std::string& filter);
    void SortItemsBy(std::function<bool(const Item&, const Item&)> comparator)
    {
        std::sort(filteredItemList.begin(), filteredItemList.end(), comparator);
    }
    
    // 선택된 항목 가져오기 위한 함수들 추가
    bool IsConfirmed() const { return isSelectionConfirmed; }
    bool IsCanceled() const { return !isOpen && !isSelectionConfirmed; }
    T GetSelected() const
    {
        if (selectedIndex >= 0 && selectedIndex < filteredItemList.size())
            return filteredItemList[selectedIndex].value;
        return T();
    }

private:
    std::vector<Item> itemList;
    std::vector<Item> filteredItemList;
    size_t selectedIndex;
    bool isSelectionConfirmed;
    bool isOpen;
    bool isDialog;

};


template <typename T>
void SelectorUI<T>::UpdateImGui()
{
    constexpr size_t FILTER_BUFFER_SIZE = 256;
    char filterBuffer[FILTER_BUFFER_SIZE] = "";
    ImGui::Text("Search:");
    ImGui::SameLine();
    if (ImGui::InputText("##Filter", filterBuffer, FILTER_BUFFER_SIZE))
    {
        UpdateFilteredItems(filterBuffer);
    }

    const float listHeight = ImGui::GetContentRegionAvail().y - 40;

    ImGui::BeginChild("##ScrollingRegion", ImVec2(0, listHeight), true, ImGuiWindowFlags_HorizontalScrollbar);

    for (size_t i = 0; i < filteredItemList.size(); i++)
    {
        const Item& item = filteredItemList[i];
        const std::string& itemName = std::string(item.name.begin(), item.name.end());

        if (ImGui::Selectable(itemName.c_str(), selectedIndex == i))
        {
            selectedIndex = i;
        }
    }

    ImGui::EndChild();

    ImGui::Separator();

    if (ImGui::Button("선택", ImVec2(120, 0)))
    {
        if (selectedIndex >= 0 && selectedIndex < filteredItemList.size())
        {
            isSelectionConfirmed = true;
            isOpen = false;
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("취소", ImVec2(120, 0)))
    {
        isOpen = false;
    }
}

template <typename T>
void SelectorUI<T>::UpdateFilteredItems(const std::string& filterStr)
{
    filteredItemList.clear();
    std::string lowerFilter = filterStr;
    std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

    for (const auto& item : itemList)
    {
        std::string lowerName = std::string(item.name.begin(), item.name.end());
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

        if (lowerName.find(lowerFilter) != std::string::npos)
        {
            filteredItemList.push_back(item);
        }
    }

    selectedIndex = filteredItemList.empty() ? -1 : 0;
}