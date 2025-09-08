//
// Created by jonathan on 2025/9/8.
//

#ifndef CONNECT_OPTION_H
#define CONNECT_OPTION_H

#include <mqtt/connect_options.h>
#include <mqtt/properties.h>

class ConnectOption {
    mqtt::connect_options *m_pOptions;
    mqtt::properties *m_pProperties;

public:
    ConnectOption(mqtt::connect_options *co, mqtt::properties *props);

    ~ConnectOption();

    [[nodiscard]] mqtt::connect_options *getOption() const;
    [[nodiscard]] mqtt::properties* getProperties() const;
};

#endif //CONNECT_OPTION_H
