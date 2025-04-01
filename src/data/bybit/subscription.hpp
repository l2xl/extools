// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#ifndef SUBSCRIPTION_HPP
#define SUBSCRIPTION_HPP

#include "bybit/data_manager.hpp"

namespace scratcher::bybit {

struct ByBitSubscription
{
    const std::string symbol;

    std::shared_ptr<ByBitDataManager> dataManager;

    bool IsReady() const
    { return dataManager && dataManager->IsReadyHandleData(); }

    void Handle(const SubscriptionTopic& topic, const std::string& type, const nlohmann::json& payload)
    { if (dataManager) dataManager->HandleData(topic, type, payload); }

    void HandleError(boost::system::error_code ec)
    { if (dataManager) dataManager->HandleError(ec);}
};


}


#endif //SUBSCRIPTION_HPP
