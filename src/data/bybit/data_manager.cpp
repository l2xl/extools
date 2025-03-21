// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#include <iostream>

#include "bybit/data_manager.hpp"
#include "bybit.hpp"
#include "stream.hpp"
#include "bybit/subscription.hpp"

namespace scratcher::bybit {

ByBitDataManager::ByBitDataManager(std::string symbol, std::shared_ptr<ByBitApi> api)
    : DataProvider()
    , m_symbol(move(symbol)), mApi(move(api))
{
}

std::shared_ptr<ByBitDataManager> ByBitDataManager::Create(std::string symbol, std::shared_ptr<ByBitApi> api)
{
    auto collector = std::make_shared<ByBitDataManager>(symbol, api);
    auto subscription = api->Subscribe(symbol, collector);
    return collector;
}

void ByBitDataManager::HandleData(const SubscriptionTopic& topic, const std::string& type, const nlohmann::json& data)
{
    assert(topic.Symbol() == m_symbol);


    if (topic.Title() == "publicTrade") {
        if (!data.is_array()) throw std::invalid_argument("Invalid pablic trade data");
        if (type != "snapshot") throw std::invalid_argument("Unknown public trade message type: " + type);
        for (const auto& t: data) {
            if (!(t.contains("S") && t.contains("T") && t.contains("i") && t.contains("p") && t.contains("v"))) throw std::invalid_argument("Invalid data");
            if (t.contains("s") && t["s"] != m_symbol) throw std::invalid_argument("Trade symbol mismatch");

            std::string id = t["i"].get<std::string>();
            std::string side_str = t["S"].get<std::string>();
            TradeSide side;
            if (side_str == "Sell")
                side = TradeSide::SELL;
            else if (side_str == "Buy")
                side = TradeSide::BUY;
            else throw std::invalid_argument("Invalid trade side: " + side_str);

            time trade_time(milliseconds(t["T"].get<long>()));

            currency<uint64_t> price(0, 2);
            price.parse(t["p"].get<std::string>());

            currency<uint64_t> value(0, 8);
            value.parse(t["v"].get<std::string>());

            std::clog << side_str << ": " << trade_time << ", price (points): " << price.raw() << ", volume (points): " << value.raw() << std::endl;

            m_public_trade_cache.emplace_back(move(id), trade_time, price.raw(), value.raw(), side);
        }
    }
    else if (topic.Title() == "orderbook"){
        if (!(data.is_object() && data.contains("b") && data.contains("a"))) throw std::invalid_argument("Invalid order book data");
        if (!(data["b"].is_array() && data["a"].is_array())) throw std::invalid_argument("Invalid order book bids/asks");

        std::clog << data.dump() << std::endl;

        if (type == "snapshot") {
            m_order_book_bids.clear();
            m_order_book_asks.clear();

            for (const auto& bid: data["b"]) {
                if (!(bid.is_array() && bid.size() == 2)) throw std::invalid_argument("Wrong order book bid entry");

                currency<uint64_t> price(0, 2);
                price.parse(bid[0].get<std::string>());

                currency<uint64_t> volume(0, 8);
                volume.parse(bid[1].get<std::string>());

                m_order_book_bids.emplace_hint(m_order_book_bids.begin(), price.raw(), volume.raw());
            }
            for (const auto& ask: data["a"]) {
                if (!(ask.is_array() && ask.size() == 2)) throw std::invalid_argument("Wrong order book bid entry");

                currency<uint64_t> price(0, 2);
                price.parse(ask[0].get<std::string>());

                currency<uint64_t> volume(0, 8);
                volume.parse(ask[1].get<std::string>());

                m_order_book_bids.emplace_hint(m_order_book_bids.end(), price.raw(), volume.raw());
            }
        }
        else if (type == "delta") {
            for (const auto& bid: data["b"]) {
                if (!(bid.is_array() && bid.size() == 2)) throw std::invalid_argument("Wrong order book bid entry");

                currency<uint64_t> price(0, 2);
                price.parse(bid[0].get<std::string>());

                currency<uint64_t> volume(0, 8);
                volume.parse(bid[1].get<std::string>());

                if (volume.raw() == 0)
                    m_order_book_bids.erase(price.raw());
                else
                    m_order_book_bids[price.raw()] = volume.raw();
            }
            for (const auto& ask: data["a"]) {
                if (!(ask.is_array() && ask.size() == 2)) throw std::invalid_argument("Wrong order book bid entry");

                currency<uint64_t> price(0, 2);
                price.parse(ask[0].get<std::string>());

                currency<uint64_t> volume(0, 8);
                volume.parse(ask[1].get<std::string>());

                if (volume.raw() == 0)
                    m_order_book_asks.erase(price.raw());
                else
                    m_order_book_asks[price.raw()] = volume.raw();
            }
        }
        else throw std::invalid_argument("Unknown order book data type: " + type);
    }
}

void ByBitDataManager::HandleError(boost::system::error_code ec)
{
    std::cerr << "websock error: " << ec.message() << std::endl;

    mApi->Unsubscribe(m_symbol);
    mApi->Subscribe(m_symbol, shared_from_this());
}
} // scratcher::bybit

