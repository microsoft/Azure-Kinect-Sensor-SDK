// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AFILEPICKER_H
#define K4AFILEPICKER_H

// System headers
//
#include <array>
#include <vector>

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//
#include "filesystem17.h"

namespace k4aviewer
{
class K4AFilePicker
{
public:
    K4AFilePicker();
    bool Show();
    std17::filesystem::path GetPath();

private:
    void ChangeWorkingDirectory(std17::filesystem::path newDirectory);

    static constexpr size_t MaxPath = 4096;
    std::array<char, MaxPath> m_currentDirectoryBuffer;
    std17::filesystem::path m_selectedPath;

    std::vector<std::string> m_currentDirectoryFiles;
    std::vector<std::string> m_currentDirectorySubdirectories;

    bool m_filterExtensions = true;
};
} // namespace k4aviewer

#endif
