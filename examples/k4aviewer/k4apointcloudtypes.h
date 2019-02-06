/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4APOINTCLOUDTYPES_H
#define K4APOINTCLOUDTYPES_H

// System headers
//

// Library headers
//
#include "k4aimgui_all.h"
#include "linmath.h"

// Project headers
//

namespace k4aviewer
{
struct Vertex
{
    linmath::vec3 Position;
    linmath::vec4 Color;
    linmath::vec3 Normal;
};
} // namespace k4aviewer
#endif
