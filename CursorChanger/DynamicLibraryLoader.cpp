
#include "DynamicLibraryLoader.h"


std::unordered_map<std::wstring, HMODULE> DynamicLibraryLoader::loadedModules;
std::unordered_map<std::string, FARPROC> DynamicLibraryLoader::loadedFunctions;

DynamicLibraryLoader::DynamicLibraryLoader()
{
}

DynamicLibraryLoader::~DynamicLibraryLoader()
{
    UnloadAllLibraries();
    loadedFunctions.clear();
}

bool DynamicLibraryLoader::LoadLibraryFrom(const std::wstring& libraryName)
{
    if (loadedModules.contains(libraryName))
    {
        return true; // Already loaded
    }

    HMODULE hModule = ::LoadLibraryW(libraryName.c_str());
    if (hModule)
    {
        loadedModules[libraryName] = hModule;
        return true;
    }
    
    return false;
}

bool DynamicLibraryLoader::UnloadLibrary(const std::wstring& libraryName)
{
    auto it = loadedModules.find(libraryName);
    if (it != loadedModules.end())
    {
        FreeLibrary(it->second);
        loadedModules.erase(it);
        return true;
    }
    return false;
}

void DynamicLibraryLoader::UnloadAllLibraries()
{
    for (const auto& module : loadedModules)
    {
        FreeLibrary(module.second);
    }
    loadedModules.clear();
}

bool DynamicLibraryLoader::IsLibraryLoaded(const std::wstring& libraryName)
{
    return loadedModules.contains(libraryName);
}
