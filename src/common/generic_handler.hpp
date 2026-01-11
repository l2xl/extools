// Scratcher project
// Copyright (c) 2026 l2xl (l2xl/at/proton.me)
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

#pragma once
#include <type_traits>
#include <utility>

namespace scratcher {

template<typename DATA, typename PARENT, typename DATA_CALLABLE, typename ERROR_CALLABLE, typename ... ARGS>
class generic_handler: public PARENT
{
public:
    using data_type = DATA;
    using data_callable = std::decay_t<DATA_CALLABLE>;
    using error_callable = std::decay_t<ERROR_CALLABLE>;
private:
    data_callable m_data_handler;
    error_callable m_error_handler;

public:
    generic_handler(DATA_CALLABLE&& data_handler, ERROR_CALLABLE&& error_handler, ARGS&& ... args)
        : PARENT(std::forward<ARGS>(args)...)
        , m_data_handler(std::forward<DATA_CALLABLE>(data_handler))
        , m_error_handler(std::forward<ERROR_CALLABLE>(error_handler))
    { }

    ~generic_handler() override = default;

    void handle_data(data_type data) override
    { m_data_handler(std::forward<data_type>(data)); }

    void handle_error(std::exception_ptr eptr) override
    { m_error_handler(eptr); }
};

}
