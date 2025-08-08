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

#ifndef DB_STORAGE_HPP
#define DB_STORAGE_HPP

#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <set>

#include <SQLiteCpp/SQLiteCpp.h>

#include "scheduler.hpp"

namespace scratcher {

class IDataController;

struct TradeData {
    uint64_t timestamp;
    std::string trade_id;
    uint64_t price_points;
    uint64_t volume_points;
    std::string side;
};

class DbStorage : public std::enable_shared_from_this<DbStorage>
{
    std::unique_ptr<SQLite::Database> m_db;
    std::shared_ptr<AsioScheduler> m_scheduler;
    boost::asio::strand<boost::asio::any_io_executor> m_db_strand;

    std::set<std::shared_ptr<IDataController>> m_providers;
    
    void CreateProviderTables(const std::string& provider);
    std::string GetTradesTableName(const std::string& provider) const;

    static boost::asio::awaitable<void> coCreateInstrumentsTable(std::shared_ptr<DbStorage> self);
    static boost::asio::awaitable<void> coStoreInstrument(std::shared_ptr<DbStorage> self, std::shared_ptr<IDataController> provider);

    struct EnsurePrivate {};
public:
    explicit DbStorage(const std::string& db_path, std::shared_ptr<AsioScheduler> scheduler, EnsurePrivate);
    ~DbStorage() = default;
    
    static std::shared_ptr<DbStorage> Create(const std::string& db_path, std::shared_ptr<AsioScheduler> scheduler);
    
    // Provider management
    void BindProvider(std::shared_ptr<IDataController> provider);
    //void DeleteProvider(std::shared_ptr<IDataProvider> provider);
    //bool HasProvider(std::shared_ptr<IDataProvider> provider) const;
    
    // Instrument metadata operations
    void StoreInstrument(const std::string& provider, const std::string& symbol, const std::string& metadata_json);
    std::optional<std::string> GetInstrument(const std::string& provider, const std::string& symbol);
    
    // Trade data operations (batch)
    void StoreTrades(const std::string& provider, const std::string& symbol, const std::vector<TradeData>& trades);
    
    std::vector<TradeData> GetTrades(const std::string& provider, const std::string& symbol, uint64_t from_timestamp, uint64_t to_timestamp);
};

} // namespace scratcher

#endif // DB_STORAGE_HPP
