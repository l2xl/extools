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
#include <deque>

#include <boost/container/flat_map.hpp>

#include "currency.hpp"
#include "timedef.hpp"

namespace std {
class mutex;
}

namespace scratcher {


enum class TradeSide:uint8_t { SELL, BUY };

struct Trade
{
    std::string id;
    time_point trade_time;

    uint64_t price_points;
    uint64_t volume_points;

    TradeSide side;
};

struct Scratcher;

struct IDataProvider {
    typedef std::deque<Trade> pubtrade_cache_t;

    virtual ~IDataProvider() = default;

    virtual const std::string& Symbol() const = 0;

    virtual void AddInsctrumentDataUpdateHandler(std::function<void()> h) = 0;
    virtual void AddMarketDataUpdateHandler(std::function<void()> h) = 0;

    virtual currency<uint64_t> PricePoint() const = 0;

    virtual const pubtrade_cache_t& PublicTradeCache() const = 0;
    virtual std::mutex& PublicTradeMutex() const = 0;
    virtual const boost::container::flat_map<uint64_t, uint64_t>& Bids() const = 0;
    virtual const boost::container::flat_map<uint64_t, uint64_t>& Asks() const = 0;

    virtual std::shared_ptr<Scratcher> MakePriceRulerScratcher() const = 0;
    virtual std::shared_ptr<Scratcher> MakeQuoteGraphScratcher() const = 0;
    virtual std::shared_ptr<Scratcher> MakeOrdersSpreadScratcher() const = 0;
};


} // scratcher

#endif //DATA_PROVIDER_HPP
