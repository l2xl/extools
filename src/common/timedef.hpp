// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#ifndef TIMEDEF_HPP
#define TIMEDEF_HPP

namespace scratcher {

typedef std::chrono::utc_clock::time_point time_point;

using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::minutes;
using std::chrono::hours;
using std::chrono::days;

using std::chrono::duration_cast;

}

#endif //TIMEDEF_HPP
