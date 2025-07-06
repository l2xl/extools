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

#include <QObject>

#include "timedef.hpp"
#include "core_common.hpp"
#include "view_controller.hpp"


namespace scratcher {

class AsioScheduler;
class DataScratchWidget;
class IDataProvider;
class Scratcher;
class QuoteScratcher;


struct PendingQuotesRequest
{
    uint64_t time_start;
    uint64_t time_end;

};

class MarketViewController : public QObject, public ViewController, public std::enable_shared_from_this<MarketViewController>
{
    Q_OBJECT

    std::weak_ptr<DataScratchWidget> mWidget;
    std::shared_ptr<IDataProvider> mDataProvider;
    std::shared_ptr<AsioScheduler> mScheduler;

    duration m_end_gap;

    std::list<PendingQuotesRequest> mPendingRequests;
    std::mutex mPendingRequestsMutex;

    std::shared_ptr<Scratcher> mPriceRuler;
    std::shared_ptr<Scratcher> mPriceIndicator;
    std::shared_ptr<QuoteScratcher> mQuoteGraph;

    duration m_trade_group_time;

    struct EnsurePrivate {};

signals:
    void UpdateDataViewRect(Rectangle rect);

public:
    MarketViewController(size_t id, std::shared_ptr<DataScratchWidget> , std::shared_ptr<IDataProvider>, std::shared_ptr<AsioScheduler>, EnsurePrivate);

    static std::shared_ptr<MarketViewController> Create(size_t id, std::shared_ptr<DataScratchWidget>, std::shared_ptr<IDataProvider>, std::shared_ptr<AsioScheduler>);

    void OnDataViewChange(uint64_t view_start, uint64_t view_end);
    void OnMarketDataUpdate();

    void Update() override;
};

}


#endif //MARKET_CONTROLLER_HPP
