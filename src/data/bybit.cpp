// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#include "config.hpp"

#include "bybit.hpp"

#include <openssl/hmac.h>
#include <boost/beast/http.hpp>

namespace {

const char* const REQ_TIME = "/v5/market/time";

std::string generateSignature(const std::string &message, const std::string &secret)
{
    unsigned char* digest = HMAC(EVP_sha256(), secret.c_str(), secret.length(), (unsigned char*)message.c_str(), message.length(), NULL, NULL);

    char mdString[SHA256_DIGEST_LENGTH*2+1];
    for(int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        sprintf(&mdString[i*2], "%02x", static_cast<unsigned int>(digest[i]));

    return mdString;
}

}

ByBitApi::ByBitApi(std::shared_ptr<Config> config)
    : m_host(config->Host()), m_port(config->Port())
    , io_ctx(), ssl_ctx(ssl::context::tlsv12_client)
{
    boost::beast::ssl_stream<boost::beast::tcp_stream> sock(io_ctx, ssl_ctx);

    ip::tcp::resolver resolver(io_ctx);
    m_resolved_host = resolver.resolve(m_host, m_port);
    get_lowest_layer(sock).connect(m_resolved_host);

    if (!SSL_set_tlsext_host_name(sock.native_handle(), m_host.c_str()))
        throw std::ios_base::failure( "Failed to set SNI Hostname");

    sock.handshake(ssl::stream_base::client);

    boost::beast::http::request<boost::beast::http::string_body> req(boost::beast::http::verb::get, REQ_TIME, 11);
    req.set(boost::beast::http::field::host, m_host);

    boost::beast::http::write(sock, req);

    boost::beast::flat_buffer buf;
    boost::beast::http::response<boost::beast::http::dynamic_body> resp;
    boost::beast::http::read(sock, buf, resp);

    std::cout << "response: " << resp << std::endl;

    // m_sock.handshake(m_host, REQ_TIME);
    // if (!m_sock.is_open()) throw std::ios_base::failure("ByBit connection error");
    //
    //sock.read(m_sock_buf);
    //
    // std::clog << boost::beast::buffers_to_string(m_sock_buf.data()) << std::endl;
    //
    // m_sock.close(websock::close_code::none);
    // m_sock_buf.clear();

}
