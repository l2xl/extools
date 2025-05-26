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

#ifndef MARKET_CONTROLLER_HPP
#define MARKET_CONTROLLER_HPP

#include <list>
#include <memory>

namespace scratcher {

class DataScratchWidget;
class IDataProvider;
class Scratcher;


struct PendingQuotesRequest
{
    uint64_t time_start;
    uint64_t time_end;

};

class MarketController : public std::enable_shared_from_this<MarketController>{
    std::weak_ptr<DataScratchWidget> mWidget;
    std::shared_ptr<IDataProvider> mDataProvider;

    std::list<PendingQuotesRequest> mPendingRequests;
    std::mutex mPendingRequestsMutex;

    std::shared_ptr<Scratcher> mPriceRuler;
    std::shared_ptr<Scratcher> mQuoteGraph;

    struct EnsurePrivate {};
public:
    MarketController(std::shared_ptr<DataScratchWidget> widget, std::shared_ptr<IDataProvider> dataProvider, EnsurePrivate);

    static std::shared_ptr<MarketController> Create(std::shared_ptr<DataScratchWidget> widget, std::shared_ptr<IDataProvider> dataProvider);

    void OnDataViewChange(uint64_t view_start, uint64_t view_end);
};

}


#endif //MARKET_CONTROLLER_HPP
