// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#ifndef CURRENCY_HPP
#define CURRENCY_HPP

#include <concepts>
#include <cmath>
#include <string>

namespace scratcher {

struct fixed_point_spec { };

template <unsigned N>
struct bits : fixed_point_spec
{
    static constexpr unsigned K = (N > 0) ? 1 << (N-1) : 0;
};

template <unsigned N>
struct dec : fixed_point_spec
{
    static constexpr unsigned K = std::pow(10, N);

    // std::string to_string(auto i)
    // {
    //     if (i==0) return "0";
    //     std::string str_i =  std::to_string(i);
    //     std::ostringstream buf;
    //     if (i < K) {
    //         buf << "0.";
    //         for (size_t j = 0; j < (N - str_i.length()); ++j) buf << '0';
    //         size_t print_digits = str_i.length();
    //         for (auto d = std::div(i,10);!d.rem;d = std::div(d.quot, 10)) {
    //             --print_digits;
    //         }
    //         buf << str_i.substr(0, print_digits);
    //         return buf.str();
    //     }
    //     else {
    //         buf << str_i.substr(0, str_i.length() - N) << '.' << str_i.substr(str_i.length() - digits);
    //     }
    //
    //     std::string res = buf.str();
    //
    //     size_t cut_zeroes = 0;
    //     for (auto p = res.rbegin(); p != res.rend() && *p == '0'; ++p, ++cut_zeroes) ;
    //     if (res[res.length() - cut_zeroes - 1] == '.') ++cut_zeroes;
    //
    //     return res.substr(0, res.length() - cut_zeroes);
    // }
};

template<std::integral T, std::derived_from<fixed_point_spec> SPEC>
class currency
{
    T val;
public:
    template <std::integral I>
    constexpr currency(I v) : val(static_cast<T>(v*SPEC::K)) {}

    currency(const currency& c) = default;
    currency(currency&& c) noexcept = default;

    currency& operator=(const currency& c) = default;
    currency& operator=(currency&& c) noexcept = default;

    bool operator==(const currency& c) const = default;
    bool operator<(const currency& c) const { return val < c.val; }

    std::string to_string() const
    {
        if (!val) return "0";
        auto d = std::div(val, SPEC::K);
         std::string res = d.rem && d.quot ? (std::to_string(d.quot) + "." + std::to_string(d.rem))
                               : (d.quot ? std::to_string(d.quot) : "0." + std::to_string(d.rem));

        if (d.quot) {
            size_t cut_zeroes = 0;
            for (auto p = res.rbegin(); p != res.rend() && *p == '0'; ++p, ++cut_zeroes) ;

            return res.substr(0, res.length() - cut_zeroes);
        }
        return res;
    }

};

template <typename curr1, typename curr2>
struct trading_trait
{
    typedef curr2 price_type;
    typedef curr1 volume_type;
};

}

namespace std {

template<std::integral T, std::derived_from<scratcher::fixed_point_spec> SPEC>
std::string to_string(const scratcher::currency<T, SPEC>& c)
{ return c.to_string(); }

}

#endif //CURRENCY_HPP
