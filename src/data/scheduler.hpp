// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#ifndef WORK_SCHEDULER_HPP
#define WORK_SCHEDULER_HPP

#include <thread>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/spawn.hpp>

namespace scratcher {

enum class error:int { success, no_host_name, no_time_sync };

class xscratcher_error_category_impl : public boost::system::error_category
{
public:
    virtual ~xscratcher_error_category_impl() = default;

    static xscratcher_error_category_impl instance;
    const char* name() const noexcept override { return "xscratcher"; }
    std::string message(int ev) const override;
    char const* message(int ev, char* buffer, std::size_t len) const noexcept override;
};

inline boost::system::error_category& xscratcher_error_category()
{ return xscratcher_error_category_impl::instance; }

inline boost::system::error_code xscratcher_error_code(error e)
{ return  boost::system::error_code(static_cast<int>(e), xscratcher_error_category());}

using boost::asio::io_context;
using boost::asio::executor_work_guard;
using boost::asio::yield_context;

namespace ssl = boost::asio::ssl;

class AsioScheduler: public std::enable_shared_from_this<AsioScheduler> {
    io_context m_io_ctx;
    ssl::context m_ssl_ctx;
    executor_work_guard<io_context::executor_type> m_io_guard;
    std::thread m_thread;
public:
    AsioScheduler();
    virtual ~AsioScheduler();

    io_context& io() {return m_io_ctx; }
    ssl::context& ssl() {return m_ssl_ctx; }


};
}

#endif //WORK_SCHEDULER_HPP
