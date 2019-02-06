/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4ADATASOURCE_H
#define K4ADATASOURCE_H

// System headers
//

// Library headers
//
#include <list>

// Project headers
//
#include "ik4aobserver.h"

namespace k4aviewer
{

template<typename T> class K4ADataSource
{
public:
    K4ADataSource() = default;

    ~K4ADataSource()
    {
        NotifyTermination();
    }

    K4ADataSource(K4ADataSource &) = delete;
    K4ADataSource(K4ADataSource &&) = delete;
    K4ADataSource &operator=(K4ADataSource &) = delete;
    K4ADataSource &operator=(K4ADataSource &&) = delete;

    void RegisterObserver(std::shared_ptr<IK4AObserver<T>> &&observer)
    {
        if (m_primed)
        {
            observer->NotifyData(m_mostRecentData);
        }
        m_observers.emplace_back(std::move(observer));
    }

    void NotifyObservers(const T &data)
    {
        m_primed = true;
        m_mostRecentData = data;
        for (auto wpObserver = m_observers.begin(); wpObserver != m_observers.end();)
        {
            auto spObserver = wpObserver->lock();
            if (spObserver)
            {
                spObserver->NotifyData(data);
                ++wpObserver;
            }
            else
            {
                auto toDelete = wpObserver;
                ++wpObserver;
                m_observers.erase(toDelete);
            }
        }
    }

    void NotifyTermination()
    {
        m_primed = false;
        for (const auto &wpObserver : m_observers)
        {
            if (auto spObserver = wpObserver.lock())
            {
                spObserver->NotifyTermination();
            }
        }
    }

private:
    std::list<std::weak_ptr<IK4AObserver<T>>> m_observers;

    bool m_primed = false;
    T m_mostRecentData;
};

} // namespace k4aviewer

#endif
