//
// Created by jonathan on 9/2/25.
//

#ifndef CONNECT_OPTION_H
#define CONNECT_OPTION_H

#include <unordered_map>
#include <mutex>
#include <set>

#include <mqtt/connect_options.h>

#define MAX_INDEX 10000

class ConnectOptionMgr {
    std::unordered_map<int, mqtt::connect_options *> m_options;
    std::set<int> m_indexUsed;
    std::mutex m_mutex;

public:
    ~ConnectOptionMgr();

    int make();

    mqtt::connect_options *getOptions(int index);

    void destroy(int index);

    void reset();

private:
    int makeIndex();
};

extern ConnectOptionMgr g_connectOptionMgr;

#endif
