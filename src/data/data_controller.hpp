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

#ifndef DATA_PROVIDER_HPP
#define DATA_PROVIDER_HPP


#include <functional>
#include <chrono>

#include <boost/container/flat_map.hpp>
#include <tbb/concurrent_vector.h>

#include "currency.hpp"
#include "bybit/entities/instrument.hpp"
#include "entities/trade.hpp"

namespace scratcher {

enum class SourceType:unsigned { CACHE, MARKET };

enum class MarketDataType:unsigned { TRADE, ORDERBOOK };

struct Scratcher;

struct IDataProvider
{
    typedef tbb::concurrent_vector<data::PublicTrade> pubtrade_cache_t;

    virtual ~IDataProvider() = default;

    virtual std::optional<std::string> Symbol() const = 0;
    virtual std::string GetInstrumentMetadata() const = 0;

    virtual bool IsReadyHandleData() const = 0;

    virtual currency<uint64_t> PricePoint() const = 0;

    virtual const pubtrade_cache_t& PublicTradeCache() const = 0;
    virtual const boost::container::flat_map<uint64_t, uint64_t>& Bids() const = 0;
    virtual const boost::container::flat_map<uint64_t, uint64_t>& Asks() const = 0;
};

struct IDataController
{
    virtual ~IDataController() = default;
    virtual const std::string& Name() const = 0;

    virtual void SubscribeInstrumentList(std::function<void(const std::deque<bybit::InstrumentInfo>&)> handler) = 0;

    //virtual std::shared_ptr<IDataProvider> GetDataProvider(const std::string& id) = 0;
    // virtual void AddInsctrumentDataHandler(std::function<void(const std::string&, SourceType)> h) = 0;
    // virtual void AddNewTradeHandler(std::function<void(const std::string&, SourceType)> h) = 0;
    // virtual void AddOrderBookUpdateHandler(std::function<void(const std::string&, SourceType)> h) = 0;
};


} // scratcher

#endif //DATA_PROVIDER_HPP
