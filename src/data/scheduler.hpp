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

using boost::asio::io_context;
using boost::asio::executor_work_guard;
using boost::asio::yield_context;

namespace ssl = boost::asio::ssl;

class AsioScheduler {
    io_context m_io_ctx;
    ssl::context m_ssl_ctx;
    executor_work_guard<io_context::executor_type> m_io_guard;
    std::thread m_thread;
public:
    AsioScheduler();

    io_context& io() {return m_io_ctx; }
    ssl::context& ssl() {return m_ssl_ctx; }

    // void Spawn(auto task)
    // {
    //     boost::asio::spawn(m_io_ctx,
    //         [&](yield_context yield){ task(m_io_ctx, std::move(yield)); },
    //         [](std::exception_ptr ex) { if (ex) std::rethrow_exception(ex); });
    // }

    void SpawnSSL(auto task)
    {
        boost::asio::spawn(m_io_ctx,
            [&](yield_context yield){ task(m_io_ctx, m_ssl_ctx, std::move(yield)); },
            [](std::exception_ptr ex) { if (ex) std::rethrow_exception(ex); });

    }



};
}

#endif //WORK_SCHEDULER_HPP
