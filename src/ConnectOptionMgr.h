//
// Created by jonathan on 9/2/25.
//

#ifndef CONNECT_OPTION_MGR_H
#define CONNECT_OPTION_MGR_H

#include <unordered_map>
#include <mutex>
#include <set>

#include "ConnectOption.h"

#define MAX_INDEX 10000

class ConnectOptionMgr {
    std::unordered_map<int, ConnectOption*> m_options;
    std::set<int> m_indexUsed;
    std::mutex m_mutex;

public:
    ~ConnectOptionMgr();

    int make();

    ConnectOption *getOptions(int index);

    void destroy(int index);

    void reset();

private:
    int makeIndex();
};

extern ConnectOptionMgr g_connectOptionMgr;

#endif
