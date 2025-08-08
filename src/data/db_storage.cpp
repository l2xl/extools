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

#include <stdexcept>
#include <sstream>
#include <iostream>

#include "db_storage.hpp"
#include "data_provider.hpp"

namespace scratcher {


DbStorage::DbStorage(const std::string& db_path, std::shared_ptr<AsioScheduler> scheduler, EnsurePrivate)
    : m_db(std::make_unique<SQLite::Database>(db_path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE))
    , m_scheduler(move(scheduler)), m_db_strand(make_strand(m_scheduler->io()))
{
}

std::shared_ptr<DbStorage> DbStorage::Create(const std::string& db_path, std::shared_ptr<AsioScheduler> scheduler)
{
    auto self = std::make_shared<DbStorage>(db_path, move(scheduler), EnsurePrivate{});

    co_spawn(self->m_db_strand, coCreateInstrumentsTable(self), detached);
    //TODO: load instrument from DB
    
    return self;
}

boost::asio::awaitable<void> DbStorage::coCreateInstrumentsTable(std::shared_ptr<DbStorage> self)
{
    static const std::string sql = R"(
        CREATE TABLE IF NOT EXISTS instruments (
            provider TEXT NOT NULL,
            symbol TEXT NOT NULL,
            metadata TEXT NOT NULL,
            updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
            PRIMARY KEY (provider, symbol));)";
    self->m_db->exec(sql);
}

boost::asio::awaitable<void> DbStorage::coStoreInstrument(std::shared_ptr<DbStorage> self, std::shared_ptr<IDataController> provider)
{
    // SQLite::Transaction transaction(*self->m_db);
    //
    // std::string sql = "INSERT OR REPLACE INTO instruments (provider, symbol, metadata) VALUES (?, ?, ?);";
    //
    // SQLite::Statement query(*self->m_db, sql);
    // query.bind(1, provider->Name());
    // query.bind(2, provider->Title());
    // query.bind(3, provider->GetInstrumentMetadata());
    // query.exec();
    //
    // transaction.commit();
}


void DbStorage::CreateProviderTables(const std::string& provider)
{
    std::string trades_table = GetTradesTableName(provider);
    
    // Create instruments table for this provider

    // Create trades table for this provider
    std::ostringstream trades_sql;
    trades_sql << R"(
        CREATE TABLE IF NOT EXISTS )" << trades_table << R"( (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            symbol TEXT NOT NULL,
            timestamp INTEGER NOT NULL,
            trade_id TEXT NOT NULL,
            price_points INTEGER NOT NULL,
            volume_points INTEGER NOT NULL,
            side TEXT NOT NULL,
            created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
            UNIQUE(symbol, trade_id)
        )
    )";
    m_db->exec(trades_sql.str());
    
    // Create indexes
    std::ostringstream idx_sql;
    idx_sql << "CREATE INDEX IF NOT EXISTS idx_" << provider << "_trades_symbol_timestamp ON " 
            << trades_table << "(symbol, timestamp)";
    m_db->exec(idx_sql.str());
}

std::string DbStorage::GetTradesTableName(const std::string& provider) const
{
    return provider + "_trades";
}

void DbStorage::BindProvider(std::shared_ptr<IDataController> provider)
{
    if (m_providers.find(provider) != m_providers.end()) {
        return; // Already exists
    }

    auto ref = weak_from_this();

    provider->AddInsctrumentDataHandler([=](const std::string& symbol, SourceType source) {
        if (source == SourceType::MARKET) {
            if (auto self = ref.lock()) {
                co_spawn(self->m_db_strand, coStoreInstrument(self, provider), detached);
            }
        }
    });
    
    m_providers.insert(move(provider));

}


void DbStorage::StoreInstrument(const std::string& provider, const std::string& symbol, const std::string& metadata_json)
{

}

std::optional<std::string> DbStorage::GetInstrument(const std::string& provider, const std::string& symbol)
{

    return std::nullopt;
}

void DbStorage::StoreTrades(const std::string& provider, const std::string& symbol, const std::vector<TradeData>& trades)
{
}

std::vector<TradeData> DbStorage::GetTrades(const std::string& provider, const std::string& symbol,
                                           uint64_t from_timestamp, uint64_t to_timestamp)
{

    std::vector<TradeData> result;
    

    return result;
}


} // namespace scratcher

namespace SQLite {
void assertion_failed(const char* apFile, int apLine, const char* apFunc, const char* apExpr, const char* apMsg) {
    std::cerr << "SQLite assertion failed: " << apFile << ":" << apLine << " in " << apFunc << "() - " << apExpr;
    if (apMsg) std::cerr << " (" << apMsg << ")";
    std::cerr << std::endl;
    std::abort(); // Or throw exception if you prefer
}
}
