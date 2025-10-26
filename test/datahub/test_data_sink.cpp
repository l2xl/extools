// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <future>
#include <chrono>
#include <deque>
#include <atomic>
#include <thread>
#include <iostream>

#include "scheduler.hpp"
#include "data/bybit/entities/public_trade.hpp"
#include "data/bybit/entities/response.hpp"
#include "connect/connection_context.hpp"
#include "connect/http_query.hpp"
#include "datahub/data_sink.hpp"
#include "datahub/sync_policy.hpp"


using namespace scratcher;
using namespace scratcher::datahub;

struct DataSinkTestFixture {
    std::string url = "https://api.bybit.com/v5/market/recent-trade?category=spot&symbol=BTCUSDC";
    std::shared_ptr<SQLite::Database> db;
    std::shared_ptr<AsioScheduler> scheduler;
    std::shared_ptr<connect::context> ctx;

    // Use in-memory database that persists for the lifetime of this object
    DataSinkTestFixture() : db(std::make_shared<SQLite::Database>(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE))
        , scheduler(AsioScheduler::Create(1))
        , ctx(connect::context::create(scheduler))
    {}

    ~DataSinkTestFixture() = default;
};

// Global fixture instance that persists between tests
static DataSinkTestFixture fixture;

TEST_CASE("data_sink - First run", "[datahub][bybit][public_trades][first_run]")
{
    std::atomic<bool> data_received{false};
    std::atomic<int> callback_count{0};
    std::promise<void> completion_promise;
    auto completion_future = completion_promise.get_future();

    auto btc_usdc_sink = make_data_sink<bybit::PublicTrade, &bybit::PublicTrade::execId, policy::query_on_init<bybit::PublicTrade>>
        (
            fixture.db,
            [&](std::deque<bybit::PublicTrade>&& trades, DataSource source){
                // Handle received trades from server (first run)
                std::cout << "Data sink callback called with " << trades.size() << " trades from "
                          << (source == DataSource::CACHE ? "CACHE" : "SERVER") << std::endl;
                ++callback_count;
                if (!trades.empty()) {
                    data_received = true;
                    completion_promise.set_value();
                } else {
                    std::cout << "Data sink received empty trades list" << std::endl;
                }
            },
            [&](const std::exception_ptr& e) {
                // Handle errors
                try {
                    if (e) {
                        std::rethrow_exception(e);
                    }
                } catch (const std::exception&) {
                    completion_promise.set_exception(e);
                }
            }
        );

    // Assert basic invariants
    REQUIRE(btc_usdc_sink != nullptr);

    auto entity_acceptor = btc_usdc_sink->data_acceptor<std::deque<bybit::PublicTrade>>();

    auto resp_adapter = make_data_adapter<bybit::ApiResponse<bybit::ListResult<bybit::PublicTrade>>>(
            [=](auto&& resp) {
                entity_acceptor(move(resp.result.list));
            }
        );
    auto dispatcher = make_data_dispatcher(fixture.scheduler, resp_adapter);

    auto connection = connect::http_query::create(fixture.ctx, fixture.url,
        dispatcher,
        [&completion_promise](const std::exception_ptr& e){
            std::cout << "HTTP connection error occurred" << std::endl;
            completion_promise.set_exception(e);
        });

    (*connection)();

    // Wait for data to be received from server
    auto status = completion_future.wait_for(std::chrono::seconds(15));
    REQUIRE(status == std::future_status::ready);

    // Verify first run behavior: single callback with server data
    REQUIRE(data_received.load() == true);
    REQUIRE(callback_count.load() == 1);
}

TEST_CASE("data_sink - Second run with cache", "[datahub][bybit][public_trades][second_run]")
{
    std::atomic<int> callback_count{0};
    std::vector<std::pair<int, size_t>> callback_sequence; // callback_number, trades_count
    std::mutex callback_mutex;
    std::promise<void> completion_promise;
    auto completion_future = completion_promise.get_future();

    auto btc_usdc_sink = make_data_sink<bybit::PublicTrade, &bybit::PublicTrade::execId, policy::query_on_init<bybit::PublicTrade>>
        (
            fixture.db,
            [&](std::deque<bybit::PublicTrade>&& trades, DataSource source){
                std::lock_guard<std::mutex> lock(callback_mutex);
                int current_callback = ++callback_count;
                size_t trades_size = trades.size();
                callback_sequence.emplace_back(current_callback, trades_size);

                std::cout << "Data sink callback #" << current_callback << " called with " << trades_size << " trades from "
                          << (source == DataSource::CACHE ? "CACHE" : "SERVER") << std::endl;

                // Complete after second callback (DB data + network data)
                if (current_callback == 2) {
                    completion_promise.set_value();
                }
            },
            [&](const std::exception_ptr& e) {
                // Handle errors
                try {
                    if (e) {
                        std::rethrow_exception(e);
                    }
                } catch (const std::exception&) {
                    completion_promise.set_exception(e);
                }
            }
        );

    // Assert basic invariants
    REQUIRE(btc_usdc_sink != nullptr);

    auto entity_acceptor = btc_usdc_sink->data_acceptor<std::deque<bybit::PublicTrade>>();

    auto resp_adapter = make_data_adapter<bybit::ApiResponse<bybit::ListResult<bybit::PublicTrade>>>(
            [=](auto&& resp) {
                entity_acceptor(move(resp.result.list));
            }
        );
    auto dispatcher = make_data_dispatcher(fixture.scheduler, resp_adapter);

    auto connection = connect::http_query::create(fixture.ctx, fixture.url,
        dispatcher,
        [&completion_promise](const std::exception_ptr& e){
            completion_promise.set_exception(e);
        });

    std::cout << "Making network call for fresh data..." << std::endl;

    (*connection)();

    // Wait for both callbacks (DB data + network data)
    auto status = completion_future.wait_for(std::chrono::seconds(15));
    REQUIRE(status == std::future_status::ready);

    // Verify second run behavior: two callbacks
    REQUIRE(callback_count.load() == 2);
    REQUIRE(callback_sequence.size() == 2);

    // First callback should have DB data (simulated)
    REQUIRE(callback_sequence[0].first == 1);
    REQUIRE(callback_sequence[0].second == 60); // Simulated DB data

    // Second callback should have network data
    REQUIRE(callback_sequence[1].first == 2);
    REQUIRE(callback_sequence[1].second == 60); // Network data

    std::cout << "Callback sequence verified: "
              << "1st callback had " << callback_sequence[0].second << " trades (from DB), "
              << "2nd callback had " << callback_sequence[1].second << " trades (from network)" << std::endl;
}

namespace SQLite {
    void assertion_failed(const char* apFile, int apLine, const char* apFunc, const char* apExpr, const char* apMsg) {
        std::cerr << "SQLite assertion failed: " << apFile << ":" << apLine << " in " << apFunc << "() - " << apExpr;
        if (apMsg) std::cerr << " (" << apMsg << ")";
        std::cerr << std::endl;
        std::abort(); // Or throw exception if you prefer
    }
}
