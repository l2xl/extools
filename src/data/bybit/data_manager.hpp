// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#ifndef DATA_COLLECTOR_HPP
#define DATA_COLLECTOR_HPP

#include <string>
#include <memory>
#include <deque>

#include <boost/system/system_error.hpp>
#include <boost/container/flat_map.hpp>

#include <nlohmann/json.hpp>

#include "data_provider.hpp"


namespace scratcher::bybit {

class ByBitApi;
class SubscriptionTopic;


class ByBitDataManager: public DataProvider, public std::enable_shared_from_this<ByBitDataManager>
{
    const std::string m_symbol;
    std::shared_ptr<ByBitApi> mApi;

    std::deque<Trade> m_public_trade_cache;

    boost::container::flat_map<uint64_t, uint64_t> m_order_book_bids;
    boost::container::flat_map<uint64_t, uint64_t> m_order_book_asks;
public:
    ByBitDataManager(std::string symbol, std::shared_ptr<ByBitApi> api);

    static std::shared_ptr<ByBitDataManager> Create(std::string symbol, std::shared_ptr<ByBitApi> api);

    void HandleData(const SubscriptionTopic& topic, const std::string& type, const nlohmann::json& data);
    void HandleError(boost::system::error_code ec);

    //void AddUpdateConsumer(std::shared_ptr<IUpdateConsumer>) override;

};

} // scratcher::bybit


#endif //DATA_COLLECTOR_HPP
