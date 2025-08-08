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

#include <nlohmann/json.hpp>

#include "data_provider.hpp"
#include "currency.hpp"
#include "bybit/entities/instrument.hpp"
#include "bybit/entities/entities.hpp"


namespace scratcher {

class Scratcher;
class DbStorage;

namespace bybit {
struct ByBitSubscription;

class ByBitApi;
class SubscriptionTopic;

class ByBitDataProvider: public IDataProvider, public std::enable_shared_from_this<ByBitDataProvider>
{
    friend class ByBitDataManager;

    const std::string m_symbol;
    std::optional<InstrumentInfo> m_instrument_metadata;

    pubtrade_cache_t m_pubtrade_cache;

    boost::container::flat_map<uint64_t, uint64_t> m_order_book_bids;
    boost::container::flat_map<uint64_t, uint64_t> m_order_book_asks;

    void Setinstrument(InstrumentInfo&& instrument)
    { m_instrument_metadata = std::move(instrument); }

    struct EnsurePrivate {};
public:
    ByBitDataProvider(std::string symbol, EnsurePrivate)
        : m_symbol(std::move(symbol)) {}

    static std::shared_ptr<ByBitDataProvider> Create(std::string symbol);

    const std::string& Symbol() const override
    { return m_symbol; }

    currency<uint64_t> PricePoint() const
    { return /*m_instrument_metadata ? m_instrument_metadata->price_point :*/ currency<uint64_t>(1, 2); }

    std::string GetInstrumentMetadata() const override;

    bool IsReadyHandleData() const override
    { return m_instrument_metadata.has_value(); }

    const pubtrade_cache_t& PublicTradeCache() const override
    { return m_pubtrade_cache; }

    const boost::container::flat_map<uint64_t, uint64_t>& Bids() const override
    { return m_order_book_bids; }

    const boost::container::flat_map<uint64_t, uint64_t>& Asks() const override
    { return m_order_book_asks; }

};

class ByBitDataManager: public IDataController, public std::enable_shared_from_this<ByBitDataManager>
{
    static const std::string BYBIT;
    std::shared_ptr<ByBitApi> mApi;
    std::shared_ptr<ByBitSubscription> mSubscription;

    std::unordered_map<std::string, std::shared_ptr<ByBitDataProvider>> m_instrument_data;
    std::shared_ptr<DbStorage> mDB;

    std::list<std::function<void(const std::string&,  SourceType)>> m_instrument_handlers;
    std::list<std::function<void(const std::string&, SourceType)>> m_trade_handlers;
    std::list<std::function<void(const std::string&, SourceType)>> m_orderbook_handlers;

    struct EnsurePrivate {};
public:
    ByBitDataManager(std::shared_ptr<ByBitApi> api, std::shared_ptr<DbStorage> db, EnsurePrivate);
    ~ByBitDataManager() override = default;

    static std::shared_ptr<ByBitDataManager> Create(std::shared_ptr<ByBitApi> api, std::shared_ptr<DbStorage> db);

    const std::string& Name() const override
    { return BYBIT; }

    std::shared_ptr<IDataProvider> GetDataProvider(const std::string& id) override;

    template<typename Container>
    requires std::same_as<typename Container::value_type, bybit::InstrumentInfo>
    void ConsumeInstrumentsData(Container&& instruments)
    {
        std::clog << "Processing " << instruments.size() << " spot instruments" << std::endl;
        
        while (!instruments.empty()) {
            auto instrument = std::move(instruments.back());
            instruments.pop_back();
            
            // Create data provider if it doesn't exist
            if (m_instrument_data.find(instrument.symbol) == m_instrument_data.end()) {
                m_instrument_data[instrument.symbol] = ByBitDataProvider::Create(instrument.symbol);
            }
            
            // Move the instrument data to the provider
            m_instrument_data[instrument.symbol]->Setinstrument(std::move(instrument));
            
            // Call handlers
            for (const auto &h: m_instrument_handlers)
                h(instrument.symbol, SourceType::MARKET);
        }
    }

    void HandlePublicTradeSnapshot(const PaginatedResult<PublicTrade>& data);

    void HandleSubscriptionData(const SubscriptionTopic& topic, const std::string& type, const glz::json_t& data);
    void HandleError(boost::system::error_code ec);

    void AddInsctrumentDataHandler(std::function<void(const std::string&, SourceType)> h) override;
    void AddNewTradeHandler(std::function<void(const std::string&, SourceType)> h) override
    { m_trade_handlers.emplace_back(std::move(h)); }
    void AddOrderBookUpdateHandler(std::function<void(const std::string&, SourceType)> h) override
    { m_orderbook_handlers.emplace_back(std::move(h)); }
};

} // scratcher::bybit
}

#endif //DATA_COLLECTOR_HPP
