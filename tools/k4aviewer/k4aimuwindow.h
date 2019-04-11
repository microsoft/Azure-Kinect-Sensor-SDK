// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

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
#include "ik4avisualizationwindow.h"
#include "k4aimugraphdatagenerator.h"
#include "k4aimugraph.h"

namespace k4aviewer
{
class K4AImuWindow : public IK4AVisualizationWindow
{
public:
    void Show(K4AWindowPlacementInfo placementInfo) override;

    const char *GetTitle() const override;

    K4AImuWindow(std::string &&title, std::shared_ptr<K4AImuGraphDataGenerator> graphDataGenerator);
    ~K4AImuWindow() override = default;

    K4AImuWindow(const K4AImuWindow &) = delete;
    K4AImuWindow &operator=(const K4AImuWindow &) = delete;
    K4AImuWindow(const K4AImuWindow &&) = delete;
    K4AImuWindow &operator=(const K4AImuWindow &&) = delete;

private:
    std::shared_ptr<K4AImuGraphDataGenerator> m_graphDataGenerator;

    std::string m_title;
    bool m_failed = false;

    K4AImuGraph m_accGraph;
    K4AImuGraph m_gyroGraph;
};
} // namespace k4aviewer

#endif
