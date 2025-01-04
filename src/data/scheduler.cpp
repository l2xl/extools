// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#include "scheduler.hpp"

namespace scratcher {

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
