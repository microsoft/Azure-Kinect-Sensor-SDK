/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

// Associated header
//
#include "k4acapture.h"

// System headers
//

// Library headers
//

// Project headers
//

using namespace k4aviewer;

K4ACapture::K4ACapture(const k4a_capture_t capture) : m_capture(capture) {}

K4ACapture::~K4ACapture()
{
    if (m_capture)
    {
        k4a_capture_release(m_capture);
    }
}

std::shared_ptr<K4AImage<K4A_IMAGE_FORMAT_DEPTH16>> K4ACapture::GetDepthImage()
{
    return K4AImageFactory::CreateK4AImage<K4A_IMAGE_FORMAT_DEPTH16>(k4a_capture_get_depth_image(m_capture));
}

std::shared_ptr<K4AImage<K4A_IMAGE_FORMAT_IR16>> K4ACapture::GetIrImage()
{
    return K4AImageFactory::CreateK4AImage<K4A_IMAGE_FORMAT_IR16>(k4a_capture_get_ir_image(m_capture));
}

float K4ACapture::GetTemperature() const
{
    return k4a_capture_get_temperature_c(m_capture);
}