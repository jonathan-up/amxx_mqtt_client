//
// Created by jonathan on 2025/9/8.
//

#include "ConnectOption.h"

ConnectOption::ConnectOption(mqtt::connect_options *co, mqtt::properties *props) {
    this->m_pOptions = co;
    this->m_pProperties = props;
}

ConnectOption::~ConnectOption() {
    delete this->m_pOptions;
    delete this->m_pProperties;
}

mqtt::connect_options *ConnectOption::getOption() const {
    return this->m_pOptions;
}

mqtt::properties *ConnectOption::getProperties() const {
    return this->m_pProperties;
}
