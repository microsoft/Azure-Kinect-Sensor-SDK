// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef IK4AOBSERVER_H
#define IK4AOBSERVER_H

// System headers
//
#include <memory>

// Library headers
//
#include <k4a/k4a.hpp>

// Project headers
//

namespace k4aviewer
{
template<typename NotificationType> class IK4AObserver
{
public:
    virtual void NotifyData(const NotificationType &data) = 0;
    virtual void NotifyTermination() = 0;
    virtual void ClearData() = 0;

    virtual ~IK4AObserver() = default;

    IK4AObserver() = default;
    IK4AObserver(const IK4AObserver &) = delete;
    IK4AObserver(const IK4AObserver &&) = delete;
    IK4AObserver &operator=(const IK4AObserver &) = delete;
    IK4AObserver &operator=(const IK4AObserver &&) = delete;
};

using IK4ACaptureObserver = IK4AObserver<k4a::capture>;
using IK4AImuObserver = IK4AObserver<k4a_imu_sample_t>;
} // namespace k4aviewer

#endif
