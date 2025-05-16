#pragma once
#include <string>
#include <vector>

class FileSelector
{
public:
    FileSelector() = delete;
    FileSelector(const FileSelector&) = delete;
    FileSelector& operator=(const FileSelector&) = delete;
    FileSelector(FileSelector&&) = delete;
    FileSelector& operator=(FileSelector&&) = delete;
    ~FileSelector() = delete;
    
    static std::string OpenFileSelectDialog(const std::string& filterName = "", const std::vector<std::string>& filterTypes = {});
};
