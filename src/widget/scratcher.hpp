// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#ifndef SCRATCHER_HPP
#define SCRATCHER_HPP

namespace scratcher {

class DataScratchWidget;

struct Scratcher
{
    virtual ~Scratcher() = default;

    virtual void BeforePaint(DataScratchWidget&) const = 0;
    virtual void Paint(DataScratchWidget&) const = 0;
};

}

#endif //SCRATCHER_HPP
