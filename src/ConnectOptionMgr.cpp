//
// Created by jonathan on 9/2/25.
//

#include "ConnectOptionMgr.h"

ConnectOptionMgr::~ConnectOptionMgr() {
    for (auto &[k, v]: this->m_options) {
        delete v;
    }
}

int ConnectOptionMgr::make() {
    if (const int index = this->makeIndex(); index != 0) {
        this->m_options[index] = new mqtt::connect_options{MQTTVERSION_5};

        return index;
    }
    return 0;
}

mqtt::connect_options *ConnectOptionMgr::getOptions(const int index) {
    return this->m_options[index];
}

void ConnectOptionMgr::destroy(const int index) {
    if (const auto it = this->m_options.find(index); it != this->m_options.end()) {
        delete it->second;
        this->m_indexUsed.erase(it->first);
        this->m_options.erase(it);
    }
}

void ConnectOptionMgr::reset() {
    for (auto &[k, v]: this->m_options) {
        delete v;
    }

    this->m_indexUsed.clear();
    this->m_options.clear();
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
