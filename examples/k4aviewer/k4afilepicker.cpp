/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

// Associated header
//
#include "k4afilepicker.h"

// System headers
//
#include <algorithm>
#include <sstream>

// Library headers
//

// Project headers
//

using namespace k4aviewer;

K4AFilePicker::K4AFilePicker()
{
    ChangeWorkingDirectory(std17::filesystem::current_path());
}

bool K4AFilePicker::Show()
{
    if (ImGui::InputText("Current Dir", &m_currentDirectoryBuffer[0], m_currentDirectoryBuffer.size()))
    {
        ChangeWorkingDirectory(std17::filesystem::path(&m_currentDirectoryBuffer[0]));
        return false;
    }

    if (ImGui::Button("Parent Directory"))
    {
        std17::filesystem::path currentDirectory(&m_currentDirectoryBuffer[0]);
        ChangeWorkingDirectory(currentDirectory.parent_path());
        return false;
    }

    for (const std::string &currentSubdirectory : m_currentDirectorySubdirectories)
    {
        std::stringstream labelBuilder;
        labelBuilder << "> " << currentSubdirectory;
        if (ImGui::SmallButton(labelBuilder.str().c_str()))
        {
            std17::filesystem::path newWorkingDirectory(&m_currentDirectoryBuffer[0]);
            newWorkingDirectory.append(currentSubdirectory.c_str());

            ChangeWorkingDirectory(newWorkingDirectory);
            return false;
        }
    }

    bool wasSelected = false;
    for (const std::string &currentFile : m_currentDirectoryFiles)
    {
        std::stringstream labelBuilder;
        labelBuilder << "  " << currentFile;
        if (ImGui::SmallButton(labelBuilder.str().c_str()))
        {
            m_selectedPath = std17::filesystem::path(&m_currentDirectoryBuffer[0]).append(currentFile.c_str());
            wasSelected = true;
            break;
        }
    }

    return wasSelected;
}

std17::filesystem::path K4AFilePicker::GetPath()
{
    return m_selectedPath;
}

void K4AFilePicker::ChangeWorkingDirectory(std17::filesystem::path newDirectory)
{
    std::string newDirectoryStr = newDirectory.string();
    if (newDirectoryStr.size() + 1 > m_currentDirectoryBuffer.size())
    {
        return;
    }

    std::copy(&newDirectoryStr[0], &newDirectoryStr[newDirectoryStr.size() + 1], m_currentDirectoryBuffer.begin());

    m_currentDirectoryFiles.clear();
    m_currentDirectorySubdirectories.clear();

    for (auto it = std17::filesystem::directory_iterator(newDirectory);
         it != std17::filesystem::directory_iterator::end(newDirectory);
         ++it)
    {
        std::stringstream label;
        if (it->is_directory())
        {
            m_currentDirectorySubdirectories.emplace_back(it->path().filename().string());
        }
        else
        {
            m_currentDirectoryFiles.emplace_back(it->path().filename().string());
        }
    }

    // Directories are not guaranteed to be returned in sorted order on all platforms
    //
    std::sort(m_currentDirectoryFiles.begin(), m_currentDirectoryFiles.end());
    std::sort(m_currentDirectorySubdirectories.begin(), m_currentDirectorySubdirectories.end());
}
