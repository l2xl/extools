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
#include <chrono>
#include <deque>
#include <iostream>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "scheduler.hpp"
#include "data/bybit/entities/instrument.hpp"
#include "data/bybit/entities/response.hpp"
#include "connect/connection_context.hpp"
#include "connect/http_query.hpp"
#include "datahub/data_sink.hpp"


using namespace scratcher;
using namespace datahub;

static std::string http_samples[] = {
  R"({"retCode":0,"retMsg":"OK","result":{"category":"spot","list":[{"symbol":"BTCUSDC","baseCoin":"BTC","quoteCoin":"USDC","innovation":"0","status":"Trading","marginTrading":"none","stTag":"0","priceFilter":{"tickSize":"0.5","minPrice":"100","maxPrice":"999999"},"lotSizeFilter":{"basePrecision":"0.000001","quotePrecision":"0.01","minOrderQty":"0.00001","maxOrderQty":"10000","minOrderAmt":"10","maxOrderAmt":"500000"},"riskParameters":{"priceLimitRatioX":"0.05","priceLimitRatioY":"0.05"}}]},"retExtInfo":{},"time":1761520794180})",
};

struct DataModelTestFixture {
    std::string url = "https://api.bybit.com/v5/market/recent-trade?category=spot&symbol=BTCUSDC";
    std::shared_ptr<SQLite::Database> db;
    std::shared_ptr<scheduler> scheduler;
    std::shared_ptr<connect::context> ctx;

    // Use in-memory database that persists for the lifetime of this object
    DataModelTestFixture() : db(std::make_shared<SQLite::Database>(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE))
        , scheduler(scheduler::create(1))
        , ctx(connect::context::create(scheduler->io()))
    {}

    ~DataModelTestFixture() = default;
};

// Global fixture instance that persists between tests
static DataModelTestFixture fixture;

TEST_CASE("Write first", "[bybit][instruments]")
{
    auto dao = data_model<bybit::InstrumentInfo, &bybit::InstrumentInfo::symbol>::create(fixture.db);

    // Container to collect results
    std::deque<bybit::InstrumentInfo> cache = dao->query();

    auto sink = make_data_sink<bybit::InstrumentInfo>(
        dao->data_acceptor<std::deque<bybit::InstrumentInfo>>(),
        make_data_acceptor(cache),
        [](const std::exception_ptr&) {}
    );

    REQUIRE(sink != nullptr);

    auto entity_acceptor = sink->data_acceptor<std::deque<bybit::InstrumentInfoAPI>>();

    auto resp_adapter = make_data_adapter<bybit::ApiResponse<bybit::ListResult<bybit::InstrumentInfoAPI>>>(
            [entity_acceptor](auto&& resp) mutable {
                std::cout << "Adapter: Processing response with " << resp.result.list.size()
                          << " instruments" << std::endl;
                entity_acceptor(std::move(resp.result.list));
            }
        );
    auto dispatcher = make_data_dispatcher(fixture.scheduler->io().get_executor(), resp_adapter);

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

    // Wait for async processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify results
    REQUIRE(cache.size() == 1);
    std::cout << "Received " << cache.size() << " instruments" << std::endl;
}

namespace SQLite {
    void assertion_failed(const char* apFile, int apLine, const char* apFunc, const char* apExpr, const char* apMsg) {
        std::cerr << "SQLite assertion failed: " << apFile << ":" << apLine << " in " << apFunc << "() - " << apExpr;
        if (apMsg) std::cerr << " (" << apMsg << ")";
        std::cerr << std::endl;
        std::abort(); // Or throw exception if you prefer
    }
}
