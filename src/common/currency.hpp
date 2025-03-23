// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#ifndef CURRENCY_HPP
#define CURRENCY_HPP

#include <concepts>
#include <cmath>
#include <stdexcept>
#include <string>

namespace scratcher {

// struct fixed_point_spec { };
//
// template <unsigned N>
// struct bits : fixed_point_spec
// {
//     static constexpr unsigned K = (N > 0) ? 1 << (N-1) : 0;
// };
//
// template <unsigned N>
// struct dec : fixed_point_spec
// {
//     static constexpr unsigned K = std::pow(10, N);
//
//     // std::string to_string(auto i)
//     // {
//     //     if (i==0) return "0";
//     //     std::string str_i =  std::to_string(i);
//     //     std::ostringstream buf;
//     //     if (i < K) {
//     //         buf << "0.";
//     //         for (size_t j = 0; j < (N - str_i.length()); ++j) buf << '0';
//     //         size_t print_digits = str_i.length();
//     //         for (auto d = std::div(i,10);!d.rem;d = std::div(d.quot, 10)) {
//     //             --print_digits;
//     //         }
//     //         buf << str_i.substr(0, print_digits);
//     //         return buf.str();
//     //     }
//     //     else {
//     //         buf << str_i.substr(0, str_i.length() - N) << '.' << str_i.substr(str_i.length() - digits);
//     //     }
//     //
//     //     std::string res = buf.str();
//     //
//     //     size_t cut_zeroes = 0;
//     //     for (auto p = res.rbegin(); p != res.rend() && *p == '0'; ++p, ++cut_zeroes) ;
//     //     if (res[res.length() - cut_zeroes - 1] == '.') ++cut_zeroes;
//     //
//     //     return res.substr(0, res.length() - cut_zeroes);
//     // }
// };

namespace {
    size_t parse_presision_decimals(std::string_view str)
    {
        if (str.empty()) throw std::invalid_argument("empty currency string");
        size_t res = 0;
        if (auto point_pos = str.find('.'); point_pos != std::string_view::npos) {
            if (point_pos == str.length() - 1 || str.find('.', point_pos + 1) != std::string_view::npos) throw std::invalid_argument("invalid currency format: " + std::string(str));

            return str.length() - point_pos - 1;
        }
        return res;
    }
}

template<std::integral T/*, std::derived_from<fixed_point_spec> SPEC*/>
class currency
{
    static constexpr T MAX_PARSE = std::numeric_limits<T>::max()/10;
    const size_t m_decimals;
    const T m_multiplier;
    T m_value;

public:
    template <std::integral I>
    constexpr currency(I val, size_t decimals)
        : m_decimals(decimals), m_multiplier(std::pow(10, decimals)), m_value(static_cast<T>(val*m_multiplier)) {}

    explicit currency(std::string_view str) : currency(0, parse_presision_decimals(str))
    { parse(str); }

    currency(const currency& c) = default;
    //currency(currency&& c) noexcept = default;

    currency& operator=(const currency& c)
    {
        if (m_multiplier != c.m_multiplier) throw std::invalid_argument("inconsistent multiplier");
        m_value = c.m_value;
        return *this;
    }
    //currency& operator=(currency&& c) noexcept = default;

    bool operator==(const currency& c) const
    {
        if (m_multiplier == c.m_multiplier)
            return m_value == c.m_value;
        if (m_decimals < c.m_decimals)
            return (m_value * std::pow(10, c.m_decimals - m_decimals)) == c.m_value;

        return m_value == (c.m_value * std::pow(10, m_decimals - c.m_decimals));
    }

    bool operator!=(const currency& c) const
    { return !operator==(c); }

    bool operator<(const currency& c) const
    {
        if (m_multiplier == c.m_multiplier)
            return m_value < c.m_value;
        if (m_decimals < c.m_decimals)
            return (m_value * std::pow(10, c.m_decimals - m_decimals)) < c.m_value;

        return m_value < (c.m_value * std::pow(10, m_decimals - c.m_decimals));
    }

    const T& raw() const
    { return m_value; }

    size_t decimals() const
    { return m_decimals; }

    std::string to_string() const
    {
        if (!m_value) return "0";
        auto d = std::div(m_value, m_multiplier);
        std::string res = d.rem && d.quot ? (std::to_string(d.quot) + "." + std::to_string(d.rem))
                               : (d.quot ? std::to_string(d.quot) : "0." + std::to_string(d.rem));

        if (d.quot) {
            size_t cut_zeroes = 0;
            for (auto p = res.rbegin(); p != res.rend() && *p == '0'; ++p, ++cut_zeroes) ;

            return res.substr(0, res.length() - cut_zeroes);
        }
        return res;
    }

    currency& parse(std::string_view str)
    {
        static const std::string delims = ".,' ";
        bool is_decimal = false;
        size_t decimals = 0;
        T value = 0;
        for (auto c: str) {
            if (delims.find(c) != std::string::npos) {
                if (is_decimal) throw std::invalid_argument(std::string(str));
                if (c == '.') is_decimal = true;

                continue;
            }
            if (decimals > m_decimals && c != '0') throw std::overflow_error("decimals length: " + std::string(str));
            if (value > MAX_PARSE) throw std::overflow_error(std::string(str));

            value *= 10;
            if (is_decimal) ++decimals;

            if (c >= '1' && c <= '9') {
                T step = c - '1' + 1;
                if (std::numeric_limits<T>::max() - step < value) throw std::overflow_error(std::string(str));
                value += step;
            }
            else if (c != '0') throw std::invalid_argument(std::string(str));
        }

        if (decimals < m_decimals)
            value *= std::pow(10, m_decimals - decimals);
        else if (decimals > m_decimals)
            value /= std::pow(10, decimals - m_decimals);

        m_value = value;
        return *this;
    }

};


}

namespace std {

template<std::integral T>
std::string to_string(const scratcher::currency<T>& c)
{ return c.to_string(); }

}

#endif //CURRENCY_HPP
