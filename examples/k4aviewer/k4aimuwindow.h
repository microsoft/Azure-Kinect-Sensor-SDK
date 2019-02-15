/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4AIMUWINDOW_H
#define K4AIMUWINDOW_H

// System headers
//
#include <memory>
#include <string>

// Library headers
//

// Project headers
//
#include "k4aimusamplesource.h"
#include "ik4avisualizationwindow.h"
#include "k4aimudatagraph.h"

namespace k4aviewer
{
class K4AImuWindow : public IK4AVisualizationWindow
{
public:
    void Show(K4AWindowPlacementInfo placementInfo) override;

    const char *GetTitle() const override;

    K4AImuWindow(std::string &&title, std::shared_ptr<K4AImuSampleSource> sampleSource);
    ~K4AImuWindow() override = default;

    K4AImuWindow(const K4AImuWindow &) = delete;
    K4AImuWindow &operator=(const K4AImuWindow &) = delete;
    K4AImuWindow(const K4AImuWindow &&) = delete;
    K4AImuWindow &operator=(const K4AImuWindow &&) = delete;

private:
    std::shared_ptr<K4AImuSampleSource> m_sampleSource;
    std::string m_title;
    bool m_failed = false;

    K4AImuDataGraph m_accelerometerGraph;
    K4AImuDataGraph m_gyroscopeGraph;
    double m_sensorTemperature;
};
} // namespace k4aviewer

#endif
