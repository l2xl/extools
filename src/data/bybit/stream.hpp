// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#ifndef BYBIT_STREAM_HPP
#define BYBIT_STREAM_HPP

#include <chrono>
#include <atomic>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/container/flat_map.hpp>

namespace scratcher::bybit {

namespace ws = boost::beast::websocket;

using boost::asio::awaitable;

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

    using websocket = ws::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>;

    friend class ByBitApi;

    const std::weak_ptr<ByBitApi> m_api;
    const std::string m_path_spec;
    std::atomic<status> m_status;

    boost::asio::strand<websocket::executor_type> m_strand;
    std::unique_ptr<websocket> m_websock;
    boost::asio::steady_timer m_heartbeat_timer;
    std::atomic<std::chrono::system_clock::time_point> m_last_heartbeat = std::chrono::system_clock::time_point::min();

    std::atomic_uint32_t m_req_counter = 0;

    std::function<void(std::shared_ptr<ByBitStream>, std::string&&)> m_data_callback;
    std::function<void(boost::system::error_code)> m_error_callback;
    boost::container::flat_map<size_t, std::function<void()>> m_subscribe_callbacks;

    static awaitable<void> coHeartbeat(std::weak_ptr<ByBitStream> ref);
    static awaitable<void> coOpenWebSocketStream(std::shared_ptr<ByBitStream>);
    static awaitable<std::string> coReadWebSocketStream(std::shared_ptr<ByBitStream>);
    static awaitable<void> coMessage(std::shared_ptr<ByBitStream> self, std::string message);

    static std::string SubscribeMessage(const auto& topics, bool subscribe, size_t counter)
    {
        std::ostringstream buf;
        buf << R"({"req_id":")" << counter << R"(","op":")" << (subscribe ? "subscribe" : "unsubscribe") << R"(","args":[)";
        for (const auto& topic: topics)
            buf << "\"" << topic << "\",";
        buf.seekp(-1, buf.end);
        buf << "]}";
        return buf.str();
    }

    struct EnsurePrivate {};

public:
    ByBitStream(std::shared_ptr<ByBitApi> api, std::string spec,
        std::function<void(std::shared_ptr<ByBitStream>, std::string&&)> data_callback,
        std::function<void(boost::system::error_code)> error_callback, EnsurePrivate);
    ~ByBitStream();

    static std::shared_ptr<ByBitStream> Create(std::shared_ptr<ByBitApi> api, std::string path_spec,
        std::function<void(std::shared_ptr<ByBitStream>, std::string&&)> data_callback,
        std::function<void(boost::system::error_code)> error_callback);
    static awaitable<void> coExecute(std::weak_ptr<ByBitStream> ref);

    status Status() const
    { return m_status; }

    void SubscribeTopics(const auto& topics, std::function<void()> success_callback)
    {
        size_t counter = ++m_req_counter;
        m_subscribe_callbacks[counter] = std::move(success_callback);
        co_spawn(m_strand, coMessage(shared_from_this(), SubscribeMessage(topics, true, counter)), boost::asio::detached);
    }

    void UnsubscribeTopics(auto topics)
    { co_spawn(m_strand, coMessage(shared_from_this(), SubscribeMessage(topics, false, ++m_req_counter)), boost::asio::detached); }
};

}

#endif //BYBIT_STREAM_HPP
