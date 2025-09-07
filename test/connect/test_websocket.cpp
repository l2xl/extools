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

#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <string>
#include <future>
#include <chrono>

#include "scheduler.hpp"
#include "connect/websocket.hpp"
#include "connect/connection_context.hpp"
#include <glaze/glaze.hpp>

using namespace scratcher;
using namespace scratcher::connect;

TEST_CASE("websock_connection subscribe to ByBit public trades", "[connect][websocket][bybit]")
{
    // Create scheduler
    auto scheduler = AsioScheduler::Create(1);
    
    // Create connection context
    auto context = context::create(scheduler);
    
    // Create promise/future for synchronization
    std::promise<std::string> response_promise;
    auto response_future = response_promise.get_future();
    
    // Create response handler
    auto handler = [&response_promise](std::exception_ptr e, std::string message) {
        if (e) response_promise.set_exception(e);
        else
        {
            std::cout << "Received WebSocket message: " << message << std::endl;
            response_promise.set_value(message);
        }
    };
    
    // Create WebSocket connection to ByBit public stream
    auto connection = websock_connection::create(context, "wss://stream.bybit.com/v5/public/spot", handler);

    // Subscribe to BTCUSDT public trades
    // ByBit WebSocket subscription message format
    std::string subscription_message = R"({"op":"subscribe","args":["publicTrade.BTCUSDT"]})";
    
    // Setup subscription
    REQUIRE_NOTHROW((*connection)(subscription_message));
    
    // Wait for first message with timeout
    auto status = response_future.wait_for(std::chrono::seconds(5));
    REQUIRE(status == std::future_status::ready);

    // Get the response
    std::string response_body = response_future.get();

    // Verify we got a response
    REQUIRE_FALSE(response_body.empty());
}
