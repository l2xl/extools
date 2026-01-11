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

#include <iostream>
#include <sstream>

#include <glaze/glaze.hpp>

#include "data_manager.hpp"
#include "config.hpp"
#include "connect/http_query.hpp"
#include "data_controller.hpp"

namespace scratcher::bybit {
namespace {
    const std::string STREAM_PUBLIC_SPOT = "/v5/public/spot";
    const std::string API_INSTRUMENTS = "/v5/market/instruments-info?category=spot";

    std::string ping_message(size_t counter)
    {
        std::ostringstream buf;
        buf << R"({"req_id":")" << counter << R"(","op":"ping"})" ;
        return buf.str();
    }
}



const std::string ByBitDataManager::BYBIT = "ByBit";

// std::shared_ptr<ByBitDataProvider> ByBitDataProvider::Create()
// {
//     return std::make_shared<ByBitDataProvider>(EnsurePrivate());
// }
//
// std::string ByBitDataProvider::GetInstrumentMetadata() const
// {
//     if (!m_instrument.has_value()) {
//         return "{}";
//     }
//
//     return glz::write_json(*m_instrument).value();
// }

ByBitDataManager::ByBitDataManager(std::shared_ptr<scheduler> scheduler, std::shared_ptr<Config> config, std::shared_ptr<SQLite::Database> db, ensure_private)
    : m_context(connect::context::create(scheduler->io()))
    , m_db(std::move(db))
    , m_config(std::move(config))
{
    std::string ws_url = "wss://" + m_config->StreamHost() + ":" + m_config->StreamPort() + STREAM_PUBLIC_SPOT;
    m_public_stream = connect::websock_connection::create(m_context, ws_url, [](auto) {}, [](std::exception_ptr){});
    m_public_stream->set_heartbeat(std::chrono::seconds(20), ping_message);
}

std::shared_ptr<ByBitDataManager> ByBitDataManager::Create(std::shared_ptr<scheduler> scheduler, std::shared_ptr<Config> config, std::shared_ptr<SQLite::Database> db)
{
    auto self = std::make_shared<ByBitDataManager>(scheduler, std::move(config), std::move(db), ensure_private());

    // Create instrument provider (RAII: creates DB table and loads cache)
    self->m_instrument_provider = datahub::make_data_provider<InstrumentInfo, &InstrumentInfo::symbol>(self->m_db, [self](auto e){ self->HandleError(e); });

    // Subscribe to instrument updates
    std::weak_ptr<ByBitDataManager> ref = self;
    self->m_instrument_provider->subscribe([ref](const std::deque<InstrumentInfo>& instruments) {
        if (auto self = ref.lock()) {
            self->HandleInstrumentsData(instruments);
        }
    });

    // Setup data sources - HTTP query wiring
    self->SetupInstrumentDataSource();

    return self;
}



void ByBitDataManager::HandleError(std::exception_ptr eptr)
{
    try {
        std::rethrow_exception(eptr);
    } catch (const std::exception& ex) {
        std::cerr << "Instrument data source HTTP error: " << ex.what() << std::endl;
    }
}

// void ByBitDataManager::AddInsctrumentDataHandler(std::function<void(const std::string &, SourceType)> h)
// {
//     m_instrument_handlers.emplace_back(h);
//
//     // If instrument data is already received, call the handler immediately
//     for (const auto &[symbol, instrument]: m_instrument_data) {
//         if (instrument->IsReadyHandleData())
//             h(symbol, SourceType::CACHE);
//     }
// }


void ByBitDataManager::HandleInstrumentsData(const std::deque<InstrumentInfo>& instruments)
{
    std::clog << "onInstrumentsData: " << instruments.size() << " instruments" << std::endl;

    // // Update providers map (per-symbol)
    // for (const auto& instrument : instruments) {
    //     const std::string& symbol = instrument.symbol;
    //
    //     if (m_instrument_data.find(symbol) == m_instrument_data.end()) {
    //         m_instrument_data[symbol] = ByBitDataProvider::Create();
    //     }
    //     m_instrument_data[symbol]->SetInstrument(InstrumentInfo(instrument));
    //
    //     // Call per-symbol handlers
    //     for (const auto& h : m_instrument_handlers) {
    //         h(symbol, SourceType::MARKET);
    //     }
    // }
}

void ByBitDataManager::SetupInstrumentDataSource()
{
    std::string url = "https://" + m_config->HttpHost() + ":" + m_config->HttpPort() + API_INSTRUMENTS;

    std::clog << "setupInstrumentDataSource: " << url << std::endl;

    auto entity_acceptor = m_instrument_provider->data_acceptor<std::deque<InstrumentInfoAPI>>();

    auto resp_adapter = datahub::make_data_adapter<ApiResponse<ListResult<InstrumentInfoAPI>>>(
        [entity_acceptor](ApiResponse<ListResult<InstrumentInfoAPI>>&& response) mutable {
            std::clog << "Received " << response.result.list.size() << " instruments from server" << std::endl;

            entity_acceptor(std::move(response.result.list));
        }
    );

    auto dispatcher = datahub::make_data_dispatcher(m_context->io().get_executor(), std::move(resp_adapter));

    auto ref = weak_from_this();

    m_instruments_query = connect::http_query::create(
        m_context,
        url,
        std::move(dispatcher),
        [ref](std::exception_ptr e) {
            if (auto self = ref.lock())
                self->HandleError(e);
        }
    );

    (*m_instruments_query)();  // Execute the query
}

} // scratcher::bybit
