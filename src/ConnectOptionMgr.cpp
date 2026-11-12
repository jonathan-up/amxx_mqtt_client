//
// Created by jonathan on 9/2/25.
//

#include "ConnectOptionMgr.h"

#include <cassert>
#include <limits>

void ConnectOptionMgr::assertGameThread() const {
    assert(std::this_thread::get_id() == m_gameThread);
}

int ConnectOptionMgr::make() {
    assertGameThread();
    if (m_options.size() >= maxOptions) {
        throw mqtt::exception{0, "too many mqtt connect options"};
    }
    if (m_nextHandle > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())) {
        throw mqtt::exception{0, "mqtt connect options handles exhausted"};
    }

    const int handle = static_cast<int>(m_nextHandle);
    // Keep construction exception-safe until ConnectOption takes ownership.
    auto options = std::make_unique<mqtt::connect_options>(MQTTVERSION_5);
    auto properties = std::make_unique<mqtt::properties>();
    auto entry = std::make_unique<ConnectOption>(options.get(), properties.get());
    options.release();
    properties.release();
    m_options.emplace(handle, std::move(entry));
    ++m_nextHandle;
    return handle;
}

ConnectOption *ConnectOptionMgr::getOptions(const int handle) const {
    assertGameThread();
    const auto it = m_options.find(handle);
    return it == m_options.end() ? nullptr : it->second.get();
}

void ConnectOptionMgr::destroy(const int handle) {
    assertGameThread();
    m_options.erase(handle);
}

void ConnectOptionMgr::reset() {
    assertGameThread();
    m_options.clear();
}

ConnectOptionMgr g_connectOptionMgr;
