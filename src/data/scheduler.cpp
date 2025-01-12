// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#include "scheduler.hpp"

namespace scratcher {

namespace {

const char* xscratcher_error__messages[] = {"success", "no host name", "no time sync"};
std::string xscratcher_error_prefix = "XScratcher error: ";

}

xscratcher_error_category_impl xscratcher_error_category_impl::instance;

std::string xscratcher_error_category_impl::message(int e) const
{
    if (e < std::size(xscratcher_error__messages)) {
        return xscratcher_error_prefix + xscratcher_error__messages[e];
    }
    return "XScratcher unknown error: " + std::to_string(e);
}

char const * xscratcher_error_category_impl::message(int e, char *buffer, std::size_t len) const noexcept
{
    if (e < std::size(xscratcher_error__messages)) {
        std::snprintf(buffer, len, "%s: %s", xscratcher_error_prefix.c_str(), xscratcher_error__messages[e]);
    } else {
        std::snprintf(buffer, len, "XScratcher unknown error: %d", e);
    }
    return buffer;
}

AsioScheduler::AsioScheduler()
    : m_io_ctx(), m_ssl_ctx(ssl::context::tlsv12_client)
    , m_io_guard(make_work_guard(m_io_ctx))
    , m_thread([this]{m_io_ctx.run();})
{
}

AsioScheduler::~AsioScheduler()
{
    m_io_guard.reset();
    m_thread.join();
}
}
