// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the Intellectual Property Reserve License (IPRL)
// -----BEGIN PGP PUBLIC KEY BLOCK-----
//
// mDMEYdxcVRYJKwYBBAHaRw8BAQdAfacBVThCP5QDPEgSbSIudtpJS4Y4Imm5dzaN
// lM1HTem0IkwyIFhsIChsMnhsKSA8bDJ4bEBwcm90b25tYWlsLmNvbT6IkAQTFggA
// OBYhBKRCfUyWnduCkisNl+WRcOaCK79JBQJh3FxVAhsDBQsJCAcCBhUKCQgLAgQW
// AgMBAh4BAheAAAoJEOWRcOaCK79JDl8A/0/AjYVbAURZJXP3tHRgZyYyN9txT6mW
// 0bYCcOf0rZ4NAQDoFX4dytPDvcjV7ovSQJ6dzvIoaRbKWGbHRCufrm5QBA==
// =KKu7
// -----END PGP PUBLIC KEY BLOCK-----

#include <iostream>

#include "data_manager.hpp"
#include "bybit.hpp"
#include "stream.hpp"
#include "subscription.hpp"
#include "widget/scratch_widget.h"

namespace scratcher::bybit {

ByBitDataManager::ByBitDataManager(std::string symbol, std::shared_ptr<ByBitApi> api, EnsurePrivate)
    : m_symbol(move(symbol)), mApi(move(api))
{
}

std::shared_ptr<ByBitDataManager> ByBitDataManager::Create(std::string symbol, std::shared_ptr<ByBitApi> api)
{
    auto collector = std::make_shared<ByBitDataManager>(symbol, api, EnsurePrivate());
    collector->mSubscription = api->Subscribe(symbol, collector);
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

        currency<uint64_t> price_point(instr["priceFilter"]["tickSize"].get<std::string>());
        currency<uint64_t> price_precision(instr["lotSizeFilter"]["quotePrecision"].get<std::string>());
        currency<uint64_t> volume_point(instr["lotSizeFilter"]["basePrecision"].get<std::string>());
        currency<uint64_t> volume_precision(instr["lotSizeFilter"]["basePrecision"].get<std::string>());
        currency<uint64_t> min_volume(instr["lotSizeFilter"]["minOrderQty"].get<std::string>());
        currency<uint64_t> max_volume(instr["lotSizeFilter"]["maxOrderQty"].get<std::string>());
        currency<uint64_t> min_amount(instr["lotSizeFilter"]["minOrderAmt"].get<std::string>());
        currency<uint64_t> max_amount(instr["lotSizeFilter"]["maxOrderAmt"].get<std::string>());
        
        m_instrument_metadata = InsctrumentMetaData{ price_point, price_precision, volume_point, volume_precision, min_volume, max_volume, min_amount, max_amount};
    }

    for (const auto& h: m_instrument_handlers) h();
}

void ByBitDataManager::HandlePublicTradeSnapshot(const nlohmann::json &data)
{
    // if (data["category"] != "spot") throw WrongServerData("Wrong InstrumentsInfo category: " + data["category"].get<std::string>());
    // if (!data["list"].is_array())  throw WrongServerData("No or wrong InstrumentsInfo list");
    //
    // std::clog << "Trade snapshot" << std::endl;
    //
    // std::deque<Trade> trades;
    //
    // for (const auto& t: data["list"]) {
    //     if (t["symbol"] != m_symbol) continue;
    //
    //     if (!(t.contains("execId") && t.contains("time") && t.contains("side") && t.contains("price") && t.contains("size")))  throw std::invalid_argument("Invalid data");
    //
    //     std::string id = t["execId"].get<std::string>();
    //     std::string side_str = t["side"].get<std::string>();
    //     TradeSide side;
    //     if (side_str == "Sell")
    //         side = TradeSide::SELL;
    //     else if (side_str == "Buy")
    //         side = TradeSide::BUY;
    //     else throw std::invalid_argument("Invalid trade side: " + side_str);
    //
    //     time trade_time(milliseconds(t["time"].get<long>()));
    //
    //     currency<uint64_t> price = m_instrument_metadata->m_price_point;
    //     price.parse(t["price"].get<std::string>());
    //
    //     currency<uint64_t> value = m_instrument_metadata->m_volume_point;
    //     value.parse(t["size"].get<std::string>());
    //
    //     trades.emplace_back(move(id), trade_time, price.raw(), value.raw(), side);
    //
    //     std::clog << "trade: " << id << " - " << side_str << ": " << trade_time << ", price (points): " << price.raw() << ", volume (points): " << value.raw() << std::endl;
    // }
    //
    // std::sort(trades.begin(), trades.end(), [](const Trade& l, const Trade& r){ return std::stoll(l.id) > std::stoull(r.id); });
    //
    // for ( ; !trades.empty(); trades.pop_front()) {
    //     if (!m_pubtrade_cache.empty()) {
    //         if (std::stoull(m_pubtrade_cache.front().id) > std::stoull(trades.front().id)) {
    //             m_pubtrade_cache.
    //         }
    //     }
    //
    // }
}


void ByBitDataManager::HandleSubscriptionData(const SubscriptionTopic& topic, const std::string& type, const nlohmann::json& data)
{
    if (*topic.Symbol() != m_symbol) throw std::invalid_argument("Instrument symbol does not match: " + std::string(*topic.Symbol()));
    if (!IsReadyHandleData()) throw std::runtime_error("Instrument configuration is not ready");

    if (topic.Title() == "publicTrade") {
        if (!data.is_array()) throw std::invalid_argument("Invalid public trade data");
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

            currency<uint64_t> price = m_instrument_metadata->m_price_point;
            price.parse(t["p"].get<std::string>());

            currency<uint64_t> value = m_instrument_metadata->m_volume_point;
            value.parse(t["v"].get<std::string>());

            std::clog << "trade: " << id << " - " << side_str << ": " << trade_time << ", price (points): " << price.raw() << ", volume (points): " << value.raw() << std::endl;

            m_pubtrade_cache.emplace_back(move(id), trade_time, price.raw(), value.raw(), side);
        }
        for (const auto& h: m_trade_handlers) h();
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

                currency<uint64_t> price = m_instrument_metadata->m_price_point;
                price.parse(bid[0].get<std::string>());

                currency<uint64_t> volume = m_instrument_metadata->m_volume_point;
                volume.parse(bid[1].get<std::string>());

                m_order_book_bids.emplace_hint(m_order_book_bids.begin(), price.raw(), volume.raw());
            }
            for (const auto& ask: data["a"]) {
                if (!(ask.is_array() && ask.size() == 2)) throw std::invalid_argument("Wrong order book bid entry");

                currency<uint64_t> price = m_instrument_metadata->m_price_point;
                price.parse(ask[0].get<std::string>());

                currency<uint64_t> volume = m_instrument_metadata->m_volume_point;
                volume.parse(ask[1].get<std::string>());

                m_order_book_bids.emplace_hint(m_order_book_bids.end(), price.raw(), volume.raw());
            }
        }
        else if (type == "delta") {
            for (const auto& bid: data["b"]) {
                if (!(bid.is_array() && bid.size() == 2)) throw std::invalid_argument("Wrong order book bid entry");

                currency<uint64_t> price = m_instrument_metadata->m_price_point;
                price.parse(bid[0].get<std::string>());

                currency<uint64_t> volume = m_instrument_metadata->m_volume_point;
                volume.parse(bid[1].get<std::string>());

                if (volume.raw() == 0)
                    m_order_book_bids.erase(price.raw());
                else
                    m_order_book_bids[price.raw()] = volume.raw();
            }
            for (const auto& ask: data["a"]) {
                if (!(ask.is_array() && ask.size() == 2)) throw std::invalid_argument("Wrong order book bid entry");

                currency<uint64_t> price = m_instrument_metadata->m_price_point;
                price.parse(ask[0].get<std::string>());

                currency<uint64_t> volume = m_instrument_metadata->m_volume_point;
                volume.parse(ask[1].get<std::string>());

                if (volume.raw() == 0)
                    m_order_book_asks.erase(price.raw());
                else
                    m_order_book_asks[price.raw()] = volume.raw();
            }
        }
        else throw std::invalid_argument("Unknown order book data type: " + type);

        for (const auto& h: m_orderbook_handlers) h();
    }
}

void ByBitDataManager::HandleError(boost::system::error_code ec)
{
    std::cerr << "websock error: " << ec.message() << std::endl;

    mApi->Unsubscribe(m_symbol);
    mApi->Subscribe(m_symbol, shared_from_this());
}

void ByBitDataManager::AddInsctrumentDataHandler(std::function<void()> h)
{
    // If instrument data is already received, call the handler immediately
    if (m_instrument_metadata.has_value()) h();

    m_instrument_handlers.emplace_back(std::move(h));
}


} // scratcher::bybit

