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

#include <glaze/glaze.hpp>

#include "data_manager.hpp"
#include "bybit.hpp"
#include "db_storage.hpp"
#include "stream.hpp"
#include "subscription.hpp"
#include "common.hpp"

namespace scratcher::bybit {
const std::string ByBitDataManager::BYBIT = "ByBit";

std::shared_ptr<ByBitDataProvider> ByBitDataProvider::Create(std::string symbol)
{
    return std::make_shared<ByBitDataProvider>(move(symbol), EnsurePrivate());
}

std::string ByBitDataProvider::GetInstrumentMetadata() const
{
    if (!m_instrument_metadata.has_value()) {
        return "{}";
    }

    return glz::write_json(*m_instrument_metadata).value();
}

ByBitDataManager::ByBitDataManager(std::shared_ptr<ByBitApi> api, std::shared_ptr<DbStorage> db, EnsurePrivate)
    : mApi(move(api)), mDB(move(db))
{
}

std::shared_ptr<ByBitDataManager> ByBitDataManager::Create(std::shared_ptr<ByBitApi> api, std::shared_ptr<DbStorage> db)
{
    auto collector = std::make_shared<ByBitDataManager>(api, db, EnsurePrivate());
    //db->BindProvider(collector);

    // First fetch all spot instruments data
    api->FetchCommonData(collector);

    //    collector->mSubscription = api->Subscribe(symbol, collector);
    return collector;
}


std::shared_ptr<IDataProvider> ByBitDataManager::GetDataProvider(const std::string &symbol)
{
    auto it = m_instrument_data.find(symbol);
    return it != m_instrument_data.end() ? it->second : nullptr;
}


void ByBitDataManager::HandlePublicTradeSnapshot(const ListResult<PublicTrade> &data)
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


void ByBitDataManager::HandleSubscriptionData(const SubscriptionTopic &topic, const std::string &type,
                                              const glz::json_t &data)
{
    if (auto it = m_instrument_data.find(std::string(*topic.Symbol())); it != m_instrument_data.end()) {
        std::shared_ptr<ByBitDataProvider> dataProvider = it->second;

        if (!dataProvider->IsReadyHandleData()) throw std::runtime_error("Instrument configuration is not ready");

        if (topic.Title() == "publicTrade") {
            if (!data.is_array()) throw std::invalid_argument("Invalid public trade data");
            if (type != "snapshot") throw std::invalid_argument("Unknown public trade message type: " + type);
            for (const auto &t: data.get<glz::json_t::array_t>()) {
                if (!(t.contains("S") && t.contains("T") && t.contains("i") && t.contains("p") && t.contains("v")))
                    throw std::invalid_argument("Invalid data");
                if (t.contains("s") && t["s"].get<std::string>() != dataProvider->Symbol()) throw std::invalid_argument(
                    "Trade symbol mismatch");

                std::string id = t["i"].get<std::string>();
                std::string side_str = t["S"].get<std::string>();
                const data::OrderSide &side = data::OrderSide::select(side_str);

                //TODO: uncomment and fix

                //time trade_time(milliseconds(t["T"].get<int64_t>()));

                // currency<uint64_t> price = dataProvider->m_instrument_metadata->price_point;
                // price.parse(t["p"].get<std::string>());
                //
                // currency<uint64_t> value = dataProvider->m_instrument_metadata->volume_point;
                // value.parse(t["v"].get<std::string>());

                // std::clog << "trade: " << id << " - " << side_str << ": " << trade_time << ", price (points): " << price.raw() << ", volume (points): " << value.raw() << std::endl;
                //
                // dataProvider->m_pubtrade_cache.emplace_back(move(id), trade_time, price.raw(), value.raw(), side);
            }
            for (const auto &h: m_trade_handlers) h(dataProvider->Symbol(), SourceType::MARKET);
        } else if (topic.Title() == "orderbook") {
            if (!(data.is_object() && data.contains("b") && data.contains("a"))) throw std::invalid_argument(
                "Invalid order book data");
            if (!(data["b"].is_array() && data["a"].is_array())) throw std::invalid_argument(
                "Invalid order book bids/asks");

            //std::clog << data.dump() << std::endl;

            if (type == "snapshot") {
                dataProvider->m_order_book_bids.clear();
                dataProvider->m_order_book_asks.clear();

                for (const auto &bid: data["b"].get<glz::json_t::array_t>()) {
                    if (!bid.is_array()) throw std::invalid_argument("Wrong order book bid entry");
                    const auto &bid_array = bid.get<glz::json_t::array_t>();
                    if (bid_array.size() != 2) throw std::invalid_argument("Wrong order book bid entry");

                    //TODO: uncomment and fix

                    // currency<uint64_t> price = dataProvider->m_instrument_metadata->price_point;
                    // price.parse(bid_array[0].get<std::string>());
                    //
                    // currency<uint64_t> volume = dataProvider->m_instrument_metadata->volume_point;
                    // volume.parse(bid_array[1].get<std::string>());
                    //
                    // dataProvider->m_order_book_bids.emplace_hint(dataProvider->m_order_book_bids.begin(), price.raw(), volume.raw());
                }
                for (const auto &ask: data["a"].get<glz::json_t::array_t>()) {
                    if (!ask.is_array()) throw std::invalid_argument("Wrong order book ask entry");
                    const auto &ask_array = ask.get<glz::json_t::array_t>();
                    if (ask_array.size() != 2) throw std::invalid_argument("Wrong order book ask entry");

                    //TODO: uncomment and fix

                    // currency<uint64_t> price = dataProvider->m_instrument_metadata->price_point;
                    // price.parse(ask_array[0].get<std::string>());
                    //
                    // currency<uint64_t> volume = dataProvider->m_instrument_metadata->volume_point;
                    // volume.parse(ask_array[1].get<std::string>());
                    //
                    // dataProvider->m_order_book_asks.emplace_hint(dataProvider->m_order_book_asks.end(), price.raw(), volume.raw());
                }
            } else if (type == "delta") {
                for (const auto &bid: data["b"].get<glz::json_t::array_t>()) {
                    if (!bid.is_array()) throw std::invalid_argument("Wrong order book bid entry");
                    const auto &bid_array = bid.get<glz::json_t::array_t>();
                    if (bid_array.size() != 2) throw std::invalid_argument("Wrong order book bid entry");

                    //TODO: uncomment and fix

                    // currency<uint64_t> price = dataProvider->m_instrument_metadata->price_point;
                    // price.parse(bid_array[0].get<std::string>());
                    //
                    // currency<uint64_t> volume = dataProvider->m_instrument_metadata->volume_point;
                    // volume.parse(bid_array[1].get<std::string>());
                    //
                    // if (volume.raw() == 0)
                    //     dataProvider->m_order_book_bids.erase(price.raw());
                    // else
                    //     dataProvider->m_order_book_bids[price.raw()] = volume.raw();
                }
                for (const auto &ask: data["a"].get<glz::json_t::array_t>()) {
                    if (!ask.is_array()) throw std::invalid_argument("Wrong order book ask entry");
                    const auto &ask_array = ask.get<glz::json_t::array_t>();
                    if (ask_array.size() != 2) throw std::invalid_argument("Wrong order book ask entry");

                    //TODO: uncomment and fix

                    // currency<uint64_t> price = dataProvider->m_instrument_metadata->price_point;
                    // price.parse(ask_array[0].get<std::string>());
                    //
                    // currency<uint64_t> volume = dataProvider->m_instrument_metadata->volume_point;
                    // volume.parse(ask_array[1].get<std::string>());
                    //
                    // if (volume.raw() == 0)
                    //     dataProvider->m_order_book_asks.erase(price.raw());
                    // else
                    //     dataProvider->m_order_book_asks[price.raw()] = volume.raw();
                }
            } else throw std::invalid_argument("Unknown order book data type: " + type);

            for (const auto &h: m_orderbook_handlers) h(dataProvider->Symbol(), SourceType::MARKET);
        }
    } else {
        throw std::invalid_argument("Instrument symbol does not match: " + std::string(*topic.Symbol()));
    }
}

void ByBitDataManager::HandleError(boost::system::error_code ec)
{
    std::cerr << "websock error: " << ec.message() << std::endl;

    // mApi->Unsubscribe(m_symbol);
    // mApi->Subscribe(m_symbol, shared_from_this());
}

void ByBitDataManager::AddInsctrumentDataHandler(std::function<void(const std::string &, SourceType)> h)
{
    m_instrument_handlers.emplace_back(h);

    // If instrument data is already received, call the handler immediately
    for (const auto &[symbol, instrument]: m_instrument_data) {
        if (instrument->IsReadyHandleData())
            h(symbol, SourceType::CACHE);
    }
}
} // scratcher::bybit
