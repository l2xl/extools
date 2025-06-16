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

#include <catch2/catch_test_macros.hpp>

#include "currency.hpp"

using namespace scratcher;

TEST_CASE("Construct")
{
    currency<int> val1(10, 1);
    CHECK(val1.raw() == 10);
    CHECK(val1.decimals() == 1);
    CHECK(val1.multiplier() == 10);

    CHECK(currency<int>(10, 10).raw() == 10);

    CHECK(currency<size_t>(10, 1).to_string() == "1.0");
    CHECK(currency<size_t>(10, 5).to_string() == "0.00010");

    CHECK(currency<size_t>("1.00").to_string() == "1.00");
    CHECK(currency<size_t>("1.00").raw() == 100);
    CHECK(currency<size_t>("1.00").decimals() == 2);
}

TEST_CASE("Parse")
{
    currency<int64_t> val(0, 9);
    CHECK(val.raw() == 0);
    CHECK(val.decimals() == 9);
    CHECK(val.multiplier() == 1000000000);

    val.parse("1");
    CHECK(val.raw() == 1000000000);
    CHECK(val.decimals() == 9);
    CHECK(val.multiplier() == 1000000000);

    val.parse("1.0");
    CHECK(val.raw() == 1000000000);
    CHECK(val.decimals() == 9);
    CHECK(val.multiplier() == 1000000000);

    val.parse("1.0000000000");
    CHECK(val.raw() == 1000000000);
    CHECK(val.decimals() == 9);
    CHECK(val.multiplier() == 1000000000);

    val.parse("1.11");
    CHECK(val.raw() == 1110000000);
    CHECK(val.decimals() == 9);
    CHECK(val.multiplier() == 1000000000);
}
