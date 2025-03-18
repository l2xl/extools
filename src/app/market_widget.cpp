// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#include "market_widget.h"

#include <QPoint>
#include <QRect>
#include <QPen>
#include <QPainter>

#include <limits>
#include <algorithm>
#include <ranges>

namespace scratcher {
MarketWidget::MarketWidget(QWidget *parent)
    : QWidget{parent}, m_min(std::numeric_limits<double>::infinity()), m_max(std::numeric_limits<double>::infinity())
{}

namespace {

const double tick_margin_k = 0.25;
const int window_margin = 20;

}

QRect MarketWidget::getQuotesArea() const
{
    QRect r(rect());
    return QRect(r.x() + window_margin, r.y() + window_margin, r.width() - window_margin*2, r.height() - window_margin * 2);
}


void MarketWidget::calculateResetTimeScale()
{
    const static int def_tick_width = 10;
    const QRect area = getQuotesArea();

    if (def_tick_width * m_quotes.size() > area.width()) {
        m_start = m_quotes.size() - (area.width() / def_tick_width);
        m_end = m_quotes.size() + 1;
    }
    else {
        m_start = 0;
        m_end = area.width() / def_tick_width;
    }
}

void MarketWidget::calculateResetQuoteScale()
{
    if (m_start == -1 || m_end == -1) throw std::runtime_error("No time scale calculated");

    double min_quote = std::numeric_limits<double>::max();
    double max_quote = std::numeric_limits<double>::min();

    int range = std::ceil(m_end) - static_cast<size_t>(m_start);
    auto end = (m_start + range) > m_quotes.size() ? m_quotes.end() : m_quotes.begin() + range;

    for (const auto& tick: std::ranges::subrange(m_quotes.begin() + static_cast<size_t>(m_start), end)) {
        min_quote = std::min(min_quote, std::min(tick[1], tick[2]));
        max_quote = std::max(max_quote, std::max(tick[1], tick[2]));
    }

    m_min = min_quote;
    m_max = max_quote;
}

void MarketWidget::drawQuoteChart(QPainter& painter)
{
    size_t start_idx = std::floor(m_start);
    size_t end_idx = std::ceil(m_end);

    auto area = getQuotesArea();
    double tick_width = width() / (m_end - m_start);
    double tick_offset_x = area.x() + (m_start - static_cast<double>(start_idx)) * tick_width;

    QPen redPen(Qt::red, 0, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
    QPen greenPen(Qt::green, 0, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);

    for(const auto [idx, tick]: std::views::zip(std::views::iota(start_idx, end_idx), m_quotes)) {

        int y0 = area.x() + area.height() * (1 - (tick[0] - m_min) / (m_max - m_min));
        int y1 = area.x() + area.height() * (1 - (tick[1] - m_min) / (m_max - m_min));
        int y2 = area.x() + area.height() * (1 - (tick[2] - m_min) / (m_max - m_min));
        int y3 = area.x() + area.height() * (1 - (tick[3] - m_min) / (m_max - m_min));

        int x0 = tick_offset_x;
        int x1 = tick_offset_x + (tick_width * (1 - tick_margin_k) / 2);
        int x2 = tick_offset_x + (tick_width * (1 - tick_margin_k));

        painter.setPen(tick[0] <= tick[3] ? greenPen : redPen);
        painter.setBrush(tick[0] <= tick[3] ? Qt::green : Qt::red);
        painter.drawLine(QPoint(x1, y1), QPoint(x1, y2-1));
        painter.drawRect(QRect(QPoint(x0, std::min(y0, y3)), QPoint(x2-1, std::max(y0, y3)-1)));

        tick_offset_x += tick_width;
    }
}


void MarketWidget::paintEvent(QPaintEvent *event)
{
    if (m_start == -1 || m_end == -1) throw std::runtime_error("No time scale calculated");
    if (m_min == std::numeric_limits<double>::infinity() || m_max == std::numeric_limits<double>::infinity())
        throw std::runtime_error("No quote scale calculated");

    QPainter painter(this);
    drawQuoteChart(painter);
}

void MarketWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    calculateResetTimeScale();
    calculateResetQuoteScale();
}

}
