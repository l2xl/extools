// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the Intellectual Property Reserve License (IPRL)
// -----BEGIN PGP PUBLIC KEY BLOCK-----
//
// mDMEYdxcVRYJKwYBBAHaRw8BAQdAfacBVThCP5QDPEgSbSIudtpJS4Y4Imm5dzaN
// lM1HTem0IkwyIFhsIChsMnhsKSA8bDJ4bEBwcm90b21tYWlsLmNvbT6IkAQTFggA
// OBYhBKRCfUyWnduCkisNl+WRcOaCK79JBQJh3FxVAhsDBQsJCAcCBhUKCQgLAgQW
// AgMBAh4BAheAAAoJEOWRcOaCK79JDl8A/0/AjYVbAURZJXP3tHRgZyYyN9txT6mW
// 0bYCcOf0rZ4NAQDoFX4dytPDvcjV7ovSQJ6dzvIoaRbKWGbHRCufrm5QBA==
// =KKu7
// -----END PGP PUBLIC KEY BLOCK-----

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

#include <memory>
#include <iostream>
#include <SQLiteCpp/SQLiteCpp.h>
#include <boost/asio.hpp>

#include "dao.hpp"
#include "data/bybit/entities/fee_rate.hpp"

using namespace scratcher::dao;
using namespace scratcher::bybit;

// Helper to create an in-memory database for testing
class TestDatabase {
public:
    TestDatabase() 
        : io_context()
        , strand(boost::asio::make_strand(io_context))
        , db(std::make_shared<SQLite::Database>(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE))
    {
    }

    boost::asio::io_context io_context;
    boost::asio::strand<boost::asio::any_io_executor> strand;
    std::shared_ptr<SQLite::Database> db;

    // Helper to run coroutines synchronously for testing
    template<typename T>
    T sync_await(boost::asio::awaitable<T> aw) {
        std::optional<T> result;
        boost::asio::co_spawn(io_context, 
            [&]() -> boost::asio::awaitable<void> {
                result = co_await std::move(aw);
            },
            boost::asio::detached);
        
        io_context.run();
        io_context.restart();
        return std::move(*result);
    }

    // Helper for void coroutines
    void sync_await_void(boost::asio::awaitable<void> aw) {
        bool completed = false;
        boost::asio::co_spawn(io_context, 
            [&]() -> boost::asio::awaitable<void> {
                co_await std::move(aw);
                completed = true;
            },
            boost::asio::detached);
        
        io_context.run();
        io_context.restart();
        REQUIRE(completed);
    }
};

TEST_CASE("Dao<FeeRate> - Create and verify empty database", "[dao][fee_rate]") {
    TestDatabase test_db;
    
    // Create DAO with symbol as primary key
    Dao<FeeRate> dao(test_db.db, test_db.strand, "symbol");
    
    // Ensure table exists
    test_db.sync_await_void(dao.ensureTableExists());
    
    // Query empty database - should return no records
    auto all_records = test_db.sync_await(dao.queryAll());
    CHECK(all_records.empty());
    
    // Count should be 0
    auto count = test_db.sync_await(dao.count());
    CHECK(count == 0);
}

TEST_CASE("Dao<FeeRate> - Insert and query record from ByBit JSON", "[dao][fee_rate]") {
    TestDatabase test_db;
    
    // Create DAO with symbol as primary key
    Dao<FeeRate> dao(test_db.db, test_db.strand, "symbol");
    
    // Ensure table exists
    test_db.sync_await_void(dao.ensureTableExists());
    
    // ByBit API example JSON from documentation
    std::string bybit_json = R"({
        "symbol": "ETHUSDT",
        "takerFeeRate": "0.0006",
        "makerFeeRate": "0.0001"
    })";
    
    // Parse JSON into FeeRate object using Glaze
    auto parse_result = glz::read_json<FeeRate>(bybit_json);
    REQUIRE(parse_result.has_value());
    
    FeeRate fee_rate = parse_result.value();
    
    // Verify parsed data
    REQUIRE(fee_rate.symbol.has_value());
    CHECK(fee_rate.symbol.value() == "ETHUSDT");
    CHECK(fee_rate.takerFeeRate == "0.0006");
    CHECK(fee_rate.makerFeeRate == "0.0001");
    CHECK_FALSE(fee_rate.baseCoin.has_value()); // Should be empty in this example
    
    // Insert the record
    test_db.sync_await_void(dao.insert(fee_rate));
    
    // Verify insertion - count should be 1
    auto count = test_db.sync_await(dao.count());
    CHECK(count == 1);
    
    // Query all records
    auto all_records = test_db.sync_await(dao.queryAll());
    REQUIRE(all_records.size() == 1);
    
    const auto& retrieved_record = all_records[0];
    REQUIRE(retrieved_record.symbol.has_value());
    CHECK(retrieved_record.symbol.value() == "ETHUSDT");
    CHECK(retrieved_record.takerFeeRate == "0.0006");
    CHECK(retrieved_record.makerFeeRate == "0.0001");
    CHECK_FALSE(retrieved_record.baseCoin.has_value());
}

TEST_CASE("Dao<FeeRate> - Query using every method", "[dao][fee_rate]") {
    TestDatabase test_db;
    
    // Create DAO with symbol as primary key
    Dao<FeeRate> dao(test_db.db, test_db.strand, "symbol");
    
    // Ensure table exists
    test_db.sync_await_void(dao.ensureTableExists());
    
    // Create test data - multiple fee rates
    std::vector<FeeRate> test_data = {
        {
            std::string{"ETHUSDT"},   // symbol (primary key)
            std::nullopt,             // baseCoin
            "0.0006",                 // takerFeeRate
            "0.0001"                  // makerFeeRate
        },
        {
            std::string{"BTCUSDT"},   // symbol (primary key)
            std::string{"BTC"},       // baseCoin
            "0.0008",                 // takerFeeRate
            "0.0002"                  // makerFeeRate
        },
        {
            std::string{"SOLUSDT"},   // symbol (primary key)
            std::string{"SOL"},       // baseCoin
            "0.0010",                 // takerFeeRate
            "0.0005"                  // makerFeeRate
        }
    };
    
    // Insert all test data
    for (const auto& fee_rate : test_data) {
        test_db.sync_await_void(dao.insert(fee_rate));
    }
    
    SECTION("query_all() - Returns all records") {
        auto all_records = test_db.sync_await(dao.queryAll());
        CHECK(all_records.size() == 3);
    }
    
    SECTION("query() - Find specific record by symbol") {
        auto eth_condition = QueryCondition<FeeRate>::where("symbol", QueryOperator::Equal, "ETHUSDT");
        auto eth_records = test_db.sync_await(dao.query(eth_condition));
        REQUIRE(eth_records.size() == 1);
        const auto& eth_record = eth_records[0];
        REQUIRE(eth_record.symbol.has_value());
        CHECK(eth_record.symbol.value() == "ETHUSDT");
        CHECK(eth_record.takerFeeRate == "0.0006");

        auto btc_condition = QueryCondition<FeeRate>::where("symbol", QueryOperator::Equal, "BTCUSDT");
        auto btc_records = test_db.sync_await(dao.query(btc_condition));
        REQUIRE(btc_records.size() == 1);
        const auto& btc_record = btc_records[0];
        REQUIRE(btc_record.symbol.has_value());
        CHECK(btc_record.symbol.value() == "BTCUSDT");
        CHECK(btc_record.takerFeeRate == "0.0008");
    }

    SECTION("count_where() - Check record existence") {
        auto eth_condition = QueryCondition<FeeRate>::where("symbol", QueryOperator::Equal, "ETHUSDT");
        auto eth_count = test_db.sync_await(dao.count(eth_condition));
        CHECK(eth_count == 1);

        auto nonexistent_condition = QueryCondition<FeeRate>::where("symbol", QueryOperator::Equal, "NONEXISTENT");
        auto nonexistent_count = test_db.sync_await(dao.count(nonexistent_condition));
        CHECK(nonexistent_count == 0);
    }

    SECTION("count() - Total count") {
        auto total_count = test_db.sync_await(dao.count());
        CHECK(total_count == 3);
    }
    
    SECTION("update() - Update existing record") {
        // Get ETHUSDT record
        auto eth_condition = QueryCondition<FeeRate>::where("symbol", QueryOperator::Equal, "ETHUSDT");
        auto eth_records = test_db.sync_await(dao.query(eth_condition));
        REQUIRE(eth_records.size() == 1);
        auto eth_record = eth_records[0];
        
        // Modify fee rates
        eth_record.takerFeeRate = "0.0007";
        eth_record.makerFeeRate = "0.0002";
        
        // Update record
        test_db.sync_await_void(dao.update(eth_record));
        
        // Verify update
        auto updated_records = test_db.sync_await(dao.query(eth_condition));
        REQUIRE(updated_records.size() == 1);
        const auto& updated_record = updated_records[0];
        CHECK(updated_record.takerFeeRate == "0.0007");
        CHECK(updated_record.makerFeeRate == "0.0002");
    }
    
    SECTION("remove_where() - Delete specific record") {
        // Remove SOLUSDT
        auto sol_condition = QueryCondition<FeeRate>::where("symbol", QueryOperator::Equal, "SOLUSDT");
        test_db.sync_await_void(dao.remove(sol_condition));

        // Verify removal
        auto sol_count = test_db.sync_await(dao.count(sol_condition));
        CHECK(sol_count == 0);

        auto remaining_count = test_db.sync_await(dao.count());
        CHECK(remaining_count == 2);
    }
}

TEST_CASE("Dao<FeeRate> - ByBit API response with baseCoin", "[dao][fee_rate]") {
    TestDatabase test_db;
    
    // Create DAO with symbol as primary key
    Dao<FeeRate> dao(test_db.db, test_db.strand, "symbol");
    
    // Ensure table exists
    test_db.sync_await_void(dao.ensureTableExists());
    
    // ByBit API example with baseCoin (for futures/derivatives)
    std::string bybit_json_with_base = R"({
        "symbol": "BTCUSDT",
        "baseCoin": "BTC",
        "takerFeeRate": "0.0008",
        "makerFeeRate": "0.0002"
    })";
    
    // Parse JSON into FeeRate object using Glaze
    auto parse_result = glz::read_json<FeeRate>(bybit_json_with_base);
    REQUIRE(parse_result.has_value());
    
    FeeRate fee_rate = parse_result.value();
    
    // Verify parsed data
    REQUIRE(fee_rate.symbol.has_value());
    CHECK(fee_rate.symbol.value() == "BTCUSDT");
    REQUIRE(fee_rate.baseCoin.has_value());
    CHECK(fee_rate.baseCoin.value() == "BTC");
    CHECK(fee_rate.takerFeeRate == "0.0008");
    CHECK(fee_rate.makerFeeRate == "0.0002");
    
    // Insert and verify
    test_db.sync_await_void(dao.insert(fee_rate));
    
    auto btc_condition = QueryCondition<FeeRate>::where("symbol", QueryOperator::Equal, "BTCUSDT");
    auto retrieved_records = test_db.sync_await(dao.query(btc_condition));
    REQUIRE(retrieved_records.size() == 1);
    const auto& retrieved = retrieved_records[0];
    REQUIRE(retrieved.baseCoin.has_value());
    CHECK(retrieved.baseCoin.value() == "BTC");
}



namespace SQLite {
void assertion_failed(const char* apFile, int apLine, const char* apFunc, const char* apExpr, const char* apMsg) {
    std::cerr << "SQLite assertion failed: " << apFile << ":" << apLine << " in " << apFunc << "() - " << apExpr;
    if (apMsg) std::cerr << " (" << apMsg << ")";
    std::cerr << std::endl;
    std::abort(); // Or throw exception if you prefer
}
}
