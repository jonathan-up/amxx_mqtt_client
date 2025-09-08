//
// Created by jonathan on 9/2/25.
//

#include "ConnectOptionMgr.h"

ConnectOptionMgr::~ConnectOptionMgr() {
    for (auto &[k, v]: this->m_builder) {
        delete v;
    }
}

int ConnectOptionMgr::make() {
    if (const int index = this->makeIndex(); index != 0) {
        this->m_builder[index] = new mqtt::connect_options_builder{};
        return index;
    }
    return 0;
}

mqtt::connect_options_builder *ConnectOptionMgr::getBuilder(const int index) {
    return this->m_builder[index];
}

void ConnectOptionMgr::destroy(const int index) {
    if (const auto it = this->m_builder.find(index); it != m_builder.end()) {
        delete it->second;
        this->m_indexUsed.erase(it->first);
        this->m_builder.erase(it);
    }
}

void ConnectOptionMgr::reset() {
    for (auto &[k, v]: this->m_builder) {
        delete v;
    }

    this->m_indexUsed.clear();
    this->m_builder.clear();
}

int ConnectOptionMgr::makeIndex() {
    std::lock_guard lock(this->m_mutex);
    for (int i = 1; i <= MAX_INDEX; i++) {
        if (this->m_indexUsed.find(i) == this->m_indexUsed.end()) {
            this->m_indexUsed.insert(i);
            return i;
        }
    }

    throw mqtt::exception{0, "no index anymore"};
}

ConnectOptionMgr g_connectOptionMgr;
