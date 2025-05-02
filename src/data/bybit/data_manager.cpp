// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#include <iostream>

#include "data_manager.hpp"
#include "bybit.hpp"
#include "stream.hpp"
#include "subscription.hpp"
#include "../../widget/quote_scratcher.hpp"
#include "widget/scratch_widget.h"

namespace scratcher::bybit {

ByBitDataManager::ByBitDataManager(std::string symbol, std::shared_ptr<ByBitApi> api)
    : m_symbol(move(symbol)), mApi(move(api))
{
}

std::shared_ptr<ByBitDataManager> ByBitDataManager::Create(std::string symbol, std::shared_ptr<ByBitApi> api)
{
    auto collector = std::make_shared<ByBitDataManager>(symbol, api);
    auto subscription = api->Subscribe(symbol, collector);
    return collector;
}

void ByBitDataManager::HandleInstrumentData(const nlohmann::json &data)
{
    if (data["category"] != "spot") throw WrongServerData("Wrong InstrumentsInfo category: " + data["category"].get<std::string>());
    if (!data["list"].is_array())  throw WrongServerData("No or wrong InstrumentsInfo list");

    for (const auto& instr: data["list"]) {
        if (instr["symbol"] != m_symbol) continue;

        if (!(instr["priceFilter"].is_object() && instr["priceFilter"].contains("tickSize"))) throw WrongServerData("No or wrong InstrumentsInfo priceFilter");

        if (!(instr["lotSizeFilter"].is_object() &&
              instr["lotSizeFilter"].contains("basePrecision") &&
              instr["lotSizeFilter"].contains("quotePrecision") &&
              instr["lotSizeFilter"].contains("minOrderQty") &&
              instr["lotSizeFilter"].contains("maxOrderQty") &&
              instr["lotSizeFilter"].contains("minOrderAmt") &&
              instr["lotSizeFilter"].contains("maxOrderAmt") ))
            throw WrongServerData("No or wrong InstrumentsInfo lotSizeFilter");

        m_price_point = currency<uint64_t>(instr["priceFilter"]["tickSize"].get<std::string>());

        m_price_precision = currency<uint64_t>(instr["lotSizeFilter"]["quotePrecision"].get<std::string>());
        m_volume_point = currency<uint64_t>(instr["lotSizeFilter"]["basePrecision"].get<std::string>());
        m_volume_precision = currency<uint64_t>(instr["lotSizeFilter"]["basePrecision"].get<std::string>());
        m_min_volume = currency<uint64_t>(instr["lotSizeFilter"]["minOrderQty"].get<std::string>());
        m_max_volume = currency<uint64_t>(instr["lotSizeFilter"]["maxOrderQty"].get<std::string>());
        m_min_amount = currency<uint64_t>(instr["lotSizeFilter"]["minOrderAmt"].get<std::string>());
        m_max_amount = currency<uint64_t>(instr["lotSizeFilter"]["maxOrderAmt"].get<std::string>());
    }

    for (const auto& h: m_instrument_handlers) h();
}

void ByBitDataManager::HandleData(const SubscriptionTopic& topic, const std::string& type, const nlohmann::json& data)
{
    if (*topic.Symbol() != m_symbol) throw std::invalid_argument("Instrument symbol does not match: " + std::string(*topic.Symbol()));
    if (!IsReadyHandleData()) throw std::runtime_error("Instrument configuration is not ready");

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

            currency<uint64_t> price = *m_price_point;
            price.parse(t["p"].get<std::string>());

            currency<uint64_t> value = *m_volume_point;
            value.parse(t["v"].get<std::string>());

            std::clog << side_str << ": " << trade_time << ", price (points): " << price.raw() << ", volume (points): " << value.raw() << std::endl;

            m_public_trade_cache.emplace_back(move(id), trade_time, price.raw(), value.raw(), side);
        }
    }
    else if (topic.Title() == "orderbook"){
        if (!(data.is_object() && data.contains("b") && data.contains("a"))) throw std::invalid_argument("Invalid order book data");
        if (!(data["b"].is_array() && data["a"].is_array())) throw std::invalid_argument("Invalid order book bids/asks");

        //std::clog << data.dump() << std::endl;

        if (type == "snapshot") {
            m_order_book_bids.clear();
            m_order_book_asks.clear();

            for (const auto& bid: data["b"]) {
                if (!(bid.is_array() && bid.size() == 2)) throw std::invalid_argument("Wrong order book bid entry");

                currency<uint64_t> price = *m_price_point;
                price.parse(bid[0].get<std::string>());

                currency<uint64_t> volume = *m_volume_point;
                volume.parse(bid[1].get<std::string>());

                m_order_book_bids.emplace_hint(m_order_book_bids.begin(), price.raw(), volume.raw());
            }
            for (const auto& ask: data["a"]) {
                if (!(ask.is_array() && ask.size() == 2)) throw std::invalid_argument("Wrong order book bid entry");

                currency<uint64_t> price = *m_price_point;
                price.parse(ask[0].get<std::string>());

                currency<uint64_t> volume = *m_volume_point;
                volume.parse(ask[1].get<std::string>());

                m_order_book_bids.emplace_hint(m_order_book_bids.end(), price.raw(), volume.raw());
            }
        }
        else if (type == "delta") {
            for (const auto& bid: data["b"]) {
                if (!(bid.is_array() && bid.size() == 2)) throw std::invalid_argument("Wrong order book bid entry");

                currency<uint64_t> price = *m_price_point;
                price.parse(bid[0].get<std::string>());

                currency<uint64_t> volume = *m_volume_point;
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

    for (const auto& h: m_marketdata_handlers) h();
}

void ByBitDataManager::HandleError(boost::system::error_code ec)
{
    std::cerr << "websock error: " << ec.message() << std::endl;

    mApi->Unsubscribe(m_symbol);
    mApi->Subscribe(m_symbol, shared_from_this());
}

void ByBitDataManager::AddInsctrumentDataUpdateHandler(std::function<void()> h)
{
    if (m_price_point && m_volume_point) h();

    m_instrument_handlers.emplace_back(std::move(h));
}

void ByBitDataManager::AddMarketDataUpdateHandler(std::function<void()> h)
{
    m_marketdata_handlers.emplace_back(std::move(h));
}

std::shared_ptr<Scratcher> ByBitDataManager::MakePriceRulerScratcher() const
{
    if (!m_price_point) throw std::runtime_error("No instrument data");

    return std::make_shared<PriceRuler>(*m_price_point);
}

std::shared_ptr<Scratcher> ByBitDataManager::MakeQuoteGraphScratcher() const
{
    return std::make_shared<QuoteScratcher>(shared_from_this());
}

std::shared_ptr<Scratcher> ByBitDataManager::MakeOrdersSpreadScratcher() const
{
    return nullptr;
}

} // scratcher::bybit

