// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#ifndef BYBIT_STREAM_HPP
#define BYBIT_STREAM_HPP

#include <chrono>
#include <deque>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/lexical_cast.hpp>

namespace scratcher::bybit {

namespace ip = boost::asio::ip;
namespace websock = boost::beast::websocket;

using boost::asio::io_context;
using boost::asio::yield_context;

class ByBitApi;

class SubscriptionTopicFormatError : public std::runtime_error
{
public:
    explicit SubscriptionTopicFormatError(const std::string& what) noexcept: std::runtime_error(what) {}
    explicit SubscriptionTopicFormatError(std::string&& what) noexcept: std::runtime_error(move(what)) {}
};


class SubscriptionTopic
{
protected:
    const std::string m_topic;
    std::string_view m_title;
    std::optional<std::string_view> m_symbol;
    std::optional<std::string_view> m_size;
public:
    explicit SubscriptionTopic(std::string title) : m_topic(move(title)), m_title(m_topic) {}
    SubscriptionTopic(const std::string &title, const std::string &symbol)
        : m_topic(title + '.' + symbol)
        , m_title(m_topic.begin(), m_topic.begin() + title.size())
        , m_symbol(std::make_optional<std::string_view>(m_topic.begin() + title.size() + 1, m_topic.end()))
    {}
    SubscriptionTopic(const std::string &title, size_t size, const std::string &symbol)
        : m_topic(title + '.' + std::to_string(size) + '.' + symbol)
        , m_title(m_topic.begin(), m_topic.begin() + title.size())
        , m_symbol(std::make_optional<std::string_view>(m_topic.end() - symbol.size(), m_topic.end()))
        , m_size(std::make_optional<std::string_view>(m_topic.begin() + title.size() + 1, m_topic.end() - symbol.size() - 1))
    {}
    SubscriptionTopic(const SubscriptionTopic&) = default;
    SubscriptionTopic(SubscriptionTopic&&) noexcept = default;

    const std::string_view& Title() const { return m_title; }
    const std::optional<std::string_view>& Symbol() const { return m_symbol; }
    std::optional<size_t> Size() const { return m_size ? std::make_optional<size_t>(boost::lexical_cast<size_t>(*m_size)) : std::optional<size_t>{}; }

    static SubscriptionTopic Parse(std::string topic);

    friend std::ostream& operator<< (std::ostream&, const SubscriptionTopic&);
};

inline std::ostream& operator<< (std::ostream& s, const SubscriptionTopic& t)
{ return s << t.m_topic; }

class ByBitStream: public std::enable_shared_from_this<ByBitStream>
{
public:
    enum class status {INIT, READY, STALE};
private:

    using websocket = websock::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>;

    friend class ByBitApi;

    const std::weak_ptr<ByBitApi> m_api;
    const std::string m_path_spec;
    std::atomic<status> m_status;

    boost::asio::strand<websocket::executor_type> m_strand;
    std::unique_ptr<websocket> m_websock;
    boost::asio::steady_timer m_heartbeat_timer;
    std::atomic<std::chrono::system_clock::time_point> m_last_heartbeat = std::chrono::system_clock::time_point::min();

    std::atomic_uint32_t m_req_counter = 0;

    std::function<void(std::string&&)> m_data_callback;
    std::function<void(boost::system::error_code)> m_error_callback;

    void Heartbeat();
    void DoOpenWebSocketStream(yield_context yield);
    void DoReadWebSocketStream(yield_context yield);

    void Message(std::string message);

    std::string SubscribeMessage(const auto& topics, bool subscribe)
    {
        std::ostringstream buf;
        buf << R"({"req_id":")" << ++m_req_counter << R"(","op":")" << (subscribe ? "subscribe" : "unsubscribe") << R"(","args":[)";
        for (const auto& topic: topics)
            buf << "\"" << topic << "\",";
        buf.seekp(-1, buf.end);
        buf << "]}";
        return buf.str();
    }

public:
    ByBitStream(std::shared_ptr<ByBitApi> api, std::string spec, std::function<void(std::string&&)> data_callback, std::function<void(boost::system::error_code)> error_callback);
    ~ByBitStream();

//    static void Create(std::shared_ptr<ByBitApi> api, std::string path_spec, std::string symbol, std::function<void(std::string&&)> callback, std::function<void(boost::system::error_code)> error_callback);
    void Spawn();

    status Status() const
    { return m_status; }

    void SubscribeTopics(const auto& topics)
    { Message(SubscribeMessage(topics, true)); }
    void UnsubscribeTopics(auto topics)
    { Message(SubscribeMessage(topics, false)); }
};

}

#endif //BYBIT_STREAM_HPP
