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

#include <memory>
#include <future>
#include <chrono>
#include <deque>
#include <atomic>
#include <iostream>

#include <catch2/catch_test_macros.hpp>

#include "scheduler.hpp"
#include "data/bybit/entities/instrument.hpp"
#include "data/bybit/entities/response.hpp"
#include "connect/connection_context.hpp"
#include "connect/http_query.hpp"
#include "datahub/data_sink.hpp"


using namespace scratcher;
using namespace scratcher::datahub;

static std::string http_samples[] = {
  R"({"retCode":0,"retMsg":"OK","result":{"category":"spot","list":[{"symbol":"BTCUSDC","baseCoin":"BTC","quoteCoin":"USDC","innovation":"0","status":"Trading","marginTrading":"none","stTag":"0","priceFilter":{"tickSize":"0.5","minPrice":"100","maxPrice":"999999"},"lotSizeFilter":{"basePrecision":"0.000001","quotePrecision":"0.01","minOrderQty":"0.00001","maxOrderQty":"10000","minOrderAmt":"10","maxOrderAmt":"500000"},"riskParameters":{"priceLimitRatioX":"0.05","priceLimitRatioY":"0.05"}}]},"retExtInfo":{},"time":1761520794180})",
};

struct DataModelTestFixture {
    std::string url = "https://api.bybit.com/v5/market/recent-trade?category=spot&symbol=BTCUSDC";
    std::shared_ptr<SQLite::Database> db;
    std::shared_ptr<AsioScheduler> scheduler;
    std::shared_ptr<connect::context> ctx;

    // Use in-memory database that persists for the lifetime of this object
    DataModelTestFixture() : db(std::make_shared<SQLite::Database>(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE))
        , scheduler(AsioScheduler::Create(1))
        , ctx(connect::context::create(scheduler))
    {}

    ~DataModelTestFixture() = default;
};

// Global fixture instance that persists between tests
static DataModelTestFixture fixture;

TEST_CASE("Write first", "[bybit][instruments]")
{
    std::atomic<bool> data_received{false};
    std::atomic<int> callback_count{0};
    std::promise<void> completion_promise;
    auto completion_future = completion_promise.get_future();

    auto btc_usdc_sink = make_data_sink<bybit::InstrumentInfo, &bybit::InstrumentInfo::symbol>
        (
            fixture.db,
            [&](std::deque<bybit::InstrumentInfo>&& entities, DataSource source){
                // Handle received trades from server (first run)
                std::cout << "Data sink callback called with " << entities.size() << " entities from "
                          << (source == DataSource::CACHE ? "CACHE" : "SERVER") << std::endl;
                ++callback_count;
                if (!entities.empty()) {
                    data_received = true;
                    completion_promise.set_value();
                } else {
                    std::cout << "Data sink received empty instrument list" << std::endl;
                }
            },
            [&](std::exception_ptr e) {
                if (e) {
                    std::cout << "✗ Data sink error callback invoked" << std::endl;
                    try {
                        completion_promise.set_exception(e);
                    } catch (...) {
                        // Promise already set, ignore
                    }
                }
            }
        );

    // Assert basic invariants
    REQUIRE(btc_usdc_sink != nullptr);

    auto entity_acceptor = btc_usdc_sink->data_acceptor<std::deque<bybit::InstrumentInfoAPI>>();

    auto resp_adapter = make_data_adapter<bybit::ApiResponse<bybit::ListResult<bybit::InstrumentInfoAPI>>>(
            [&](auto&& resp) {
                std::cout << "Adapter: Processing response with " << resp.result.list.size()
                          << " instruments" << std::endl;
                entity_acceptor(move(resp.result.list));
            }
        );
    auto dispatcher = make_data_dispatcher(fixture.scheduler, resp_adapter);

    // auto query = connect::http_query::create(fixture.ctx, "https://api.bybit.com/v5/market/instruments-info",
    //     dispatcher,
    //     [&](std::exception_ptr e){
    //         if (e) {
    //             std::cout << "✗ HTTP query error" << std::endl;
    //             try {
    //                 completion_promise.set_exception(e);
    //             } catch (...) {}
    //         }
    //     }
    // );

    std::cout << "Dispatching JSON sample..." << std::endl;
    dispatcher(http_samples[0]);

    // Wait for data to be received from server
    std::cout << "Waiting for data sink callback..." << std::endl;
    auto status = completion_future.wait_for(std::chrono::seconds(2));
    
    if (status == std::future_status::timeout) {
        std::cout << "✗ Timeout waiting for completion" << std::endl;
        std::cout << "  callback_count=" << callback_count.load() 
                  << ", data_received=" << data_received.load() << std::endl;
    } else if (status == std::future_status::ready) {
        std::cout << "✓ Completion received" << std::endl;
    }
    
    REQUIRE(status == std::future_status::ready);

    // Verify first run behavior: single callback with server data
    REQUIRE(data_received.load() == true);
    REQUIRE(callback_count.load() == 1);
}

namespace SQLite {
    void assertion_failed(const char* apFile, int apLine, const char* apFunc, const char* apExpr, const char* apMsg) {
        std::cerr << "SQLite assertion failed: " << apFile << ":" << apLine << " in " << apFunc << "() - " << apExpr;
        if (apMsg) std::cerr << " (" << apMsg << ")";
        std::cerr << std::endl;
        std::abort(); // Or throw exception if you prefer
    }
}
