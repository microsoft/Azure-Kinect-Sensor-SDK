// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef IK4ADOCKCONTROL_H
#define IK4ADOCKCONTROL_H

// System headers
//

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//

namespace k4aviewer
{

enum class K4ADockControlStatus
{
    Ok,
    ShouldClose
};

class IK4ADockControl
{
public:
    virtual K4ADockControlStatus Show() = 0;

    IK4ADockControl() = default;
    virtual ~IK4ADockControl() = default;

    IK4ADockControl(const IK4ADockControl &) = delete;
    IK4ADockControl &operator=(const IK4ADockControl &) = delete;
    IK4ADockControl(const IK4ADockControl &&) = delete;
    IK4ADockControl &operator=(const IK4ADockControl &&) = delete;
};
} // namespace k4aviewer

#endif
