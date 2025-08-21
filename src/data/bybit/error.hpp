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

#ifndef ERROR_HPP
#define ERROR_HPP

namespace scratcher::bybit {

class bybit_error_category_impl : public boost::system::error_category
{
public:
    virtual ~bybit_error_category_impl() = default;

    static bybit_error_category_impl instance;
    const char* name() const noexcept override { return "bybit"; }
    std::string message(int ev) const override { return "ByBit error: " + std::to_string(ev); }
    char const* message(int ev, char* buffer, std::size_t len) const noexcept override
    {
        std::snprintf( buffer, len, "ByBit error: %d", ev);
        return buffer;
    }
};

inline boost::system::error_category& bybit_error_category()
{ return bybit_error_category_impl::instance; }

}

#endif //ERROR_HPP
