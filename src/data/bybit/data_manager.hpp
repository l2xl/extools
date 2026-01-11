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

#ifndef DATA_COLLECTOR_HPP
#define DATA_COLLECTOR_HPP

#include <string>
#include <memory>
#include <list>
#include <unordered_map>
#include <ranges>
#include <concepts>
#include <iostream>
#include <boost/asio/detail/socket_option.hpp>

#include <boost/system/system_error.hpp>
#include <boost/container/flat_map.hpp>

#include "data_controller.hpp"
#include "currency.hpp"
#include "scheduler.hpp"
#include "bybit/entities/response.hpp"
#include "bybit/entities/instrument.hpp"
#include "bybit/entities/public_trade.hpp"
#include "connection_context.hpp"
#include "http_query.hpp"
#include "websocket.hpp"
#include "datahub/data_provider.hpp"

class Config;

namespace scratcher {

class Scratcher;

namespace bybit {

// class ByBitDataProvider: public IDataProvider, public std::enable_shared_from_this<ByBitDataProvider>
// {
//     friend class ByBitDataManager;
//
//     std::optional<InstrumentInfo> m_instrument;
//
//     pubtrade_cache_t m_pubtrade_cache;
//
//     boost::container::flat_map<uint64_t, uint64_t> m_order_book_bids;
//     boost::container::flat_map<uint64_t, uint64_t> m_order_book_asks;
//
//     void SetInstrument(InstrumentInfo&& instrument)
//     { m_instrument = std::move(instrument); }
//
//     struct EnsurePrivate {};
// public:
//     ByBitDataProvider(EnsurePrivate) {}
//
//     static std::shared_ptr<ByBitDataProvider> Create();
//
//     std::optional<std::string> Symbol() const override
//     { return m_instrument ? m_instrument->symbol : std::optional<std::string>(); }
//
//     currency<uint64_t> PricePoint() const override
//     { return /*m_instrument_metadata ? m_instrument_metadata->price_point :*/ currency<uint64_t>(1, 2); }
//
//     std::string GetInstrumentMetadata() const override;
//
//     bool IsReadyHandleData() const override
//     { return m_instrument.has_value(); }
//
//     const pubtrade_cache_t& PublicTradeCache() const override
//     { return m_pubtrade_cache; }
//
//     const boost::container::flat_map<uint64_t, uint64_t>& Bids() const override
//     { return m_order_book_bids; }
//
//     const boost::container::flat_map<uint64_t, uint64_t>& Asks() const override
//     { return m_order_book_asks; }
//
// };

class ByBitDataManager: public IDataController, public std::enable_shared_from_this<ByBitDataManager>
{
public:
    using instrument_provider_type = datahub::data_provider<InstrumentInfo, &InstrumentInfo::symbol>;

private:
    static const std::string BYBIT;

    // Core dependencies
    std::shared_ptr<connect::context> m_context;
    std::shared_ptr<SQLite::Database> m_db;
    std::shared_ptr<Config> m_config;

    std::shared_ptr<connect::websock_connection> m_public_stream;

    std::shared_ptr<instrument_provider_type> m_instrument_provider;
    std::shared_ptr<connect::http_query> m_instruments_query;

    //std::unordered_map<std::string, std::shared_ptr<ByBitDataProvider>> m_instrument_data;

    std::list<std::function<void(const std::string&, SourceType)>> m_instrument_handlers;
    std::list<std::function<void(const std::string&, SourceType)>> m_trade_handlers;
    std::list<std::function<void(const std::string&, SourceType)>> m_orderbook_handlers;

    void HandleInstrumentsData(const std::deque<InstrumentInfo>& instruments);

    void SetupInstrumentDataSource();

    struct ensure_private {};
public:
    ByBitDataManager(std::shared_ptr<scheduler> scheduler,
                     std::shared_ptr<Config> config,
                     std::shared_ptr<SQLite::Database> db,
                     ensure_private);
    ~ByBitDataManager() = default;

    static std::shared_ptr<ByBitDataManager> Create(std::shared_ptr<scheduler> scheduler,
                                                    std::shared_ptr<Config> config,
                                                    std::shared_ptr<SQLite::Database> db);

    const std::string& Name() const override
    { return BYBIT; }

    void HandleError(std::exception_ptr eptr);

    // Per-symbol handlers
    //void AddInsctrumentDataHandler(std::function<void(const std::string&, SourceType)> h) override;

    // void AddNewTradeHandler(std::function<void(const std::string&, SourceType)> h)
    // { m_trade_handlers.emplace_back(std::move(h)); }
    //
    // void AddOrderBookUpdateHandler(std::function<void(const std::string&, SourceType)> h)
    // { m_orderbook_handlers.emplace_back(std::move(h)); }
    //
    // Subscribe to full instrument list updates
    void SubscribeInstrumentList(std::function<void(const std::deque<InstrumentInfo>&)> handler) override
    { m_instrument_provider->subscribe(std::move(handler)); }
};

} // scratcher::bybit
}

#endif //DATA_COLLECTOR_HPP
