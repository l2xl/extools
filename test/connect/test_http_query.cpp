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
#include <iostream>
#include <string>
#include <future>
#include <chrono>

#include "scheduler.hpp"
#include "connect/http_query.hpp"
#include "connect/connection_context.hpp"
#include "data/bybit/entities/response.hpp"
#include <glaze/glaze.hpp>

using namespace scratcher;
using namespace scratcher::connect;
using namespace scratcher::bybit;

TEST_CASE("http_query ping to ByBit server", "[connect][json_rpc][bybit]")
{
    // Create scheduler
    auto scheduler = AsioScheduler::Create(1);
    
    // Create connection context
    auto context = context::create(scheduler);
    
    // Create promise/future for synchronization
    std::promise<std::string> response_promise;
    auto response_future = response_promise.get_future();
    
    // Create response handler
    auto handler = [&response_promise](std::exception_ptr e, std::string response) {
        if (e) response_promise.set_exception(e);
        else
        {
            std::cout << "Received response: " << response << std::endl;
            response_promise.set_value(response);
        }
    };
    
    // Create JsonRpcConnection with handler
    http_query query(context, "https://api.bybit.com/v5/market/time", handler);
    
    // Execute ping request using full URL
    REQUIRE_NOTHROW(query());

    // Wait for response with timeout
    auto status = response_future.wait_for(std::chrono::seconds(5));
    REQUIRE(status == std::future_status::ready);

    // Get the response
    std::string response_body = response_future.get();

    // Verify we got a response
    REQUIRE_FALSE(response_body.empty());
}
