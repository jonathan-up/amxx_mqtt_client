//
// Created by jonathan on 9/2/25.
//

#ifndef CONNECT_OPTION_MGR_H
#define CONNECT_OPTION_MGR_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
#include <unordered_map>

#include "ConnectOption.h"

// Like client handles, options are owned and accessed only by the game thread.
class ConnectOptionMgr {
    std::unordered_map<int, std::unique_ptr<ConnectOption>> m_options;
    const std::thread::id m_gameThread = std::this_thread::get_id();
    // Never reset or wrap, including when plugins reload between maps.
    std::uint64_t m_nextHandle = 1;
    static constexpr std::size_t maxOptions = 10000;

    void assertGameThread() const;

public:
    int make();

    ConnectOption *getOptions(int handle) const;

    void destroy(int handle);

    void reset();
};

extern ConnectOptionMgr g_connectOptionMgr;

#endif
