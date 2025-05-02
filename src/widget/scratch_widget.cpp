// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#include "scratch_widget.h"

#include <QApplication>
#include <QPoint>
#include <QRect>
#include <QPen>
#include <QPainter>
#include <QDateTime>

#include <limits>
#include <algorithm>
#include <ranges>
#include <cmath>
#include <iostream>
#include <mutex>

namespace scratcher {
DataScratchWidget::DataScratchWidget(QWidget *parent)
    : QWidget{parent}
{}

// void DataScratchWidget::SetDataFieldRect(const Rectangle& rect)
// {
//     mDataField = rect;
//
//     if (mDataViewRect.x_end() <= mDataField.x_start())
//         mDataViewRect.x = mDataField.x_start();
//     if (mDataViewRect.x_start() >= mDataField.x_end())
//         mDataViewRect.x = mDataField.x_end() - mDataViewRect.w;
//     if (mDataViewRect.y_start() >= mDataField.y_end())
//         mDataViewRect.y = mDataField.y_end() - mDataViewRect.h;
//     if (mDataViewRect.y_end() <= mDataField.y_start())
//         mDataViewRect.h = mDataField.y_start();
//     //
//     // if (isVisible())
//     //     update();
// }

void DataScratchWidget::SetDataViewRect(const Rectangle& rect)
{
    mDataViewRect = rect;

    if (clientRect().width()) mXScale = static_cast<double>(mDataViewRect.w) / clientRect().width();
    if (clientRect().height()) mYScale = static_cast<double>(mDataViewRect.h) / clientRect().height();

    if (isVisible())
        update();
}

int DataScratchWidget::DataXToWidgetX(uint64_t x) const
{
    return (x - GetDataViewRect().x_start()) * (static_cast<double>(GetClientRect().width()) / GetDataViewRect().w);
}

int DataScratchWidget::DataYToWidgetY(uint64_t y) const
{
    return GetClientRect().height() - static_cast<int>((y - GetDataViewRect().y_start()) * (static_cast<double>(GetClientRect().height()) / GetDataViewRect().h));
}


void DataScratchWidget::AddScratcher(std::shared_ptr<Scratcher> s, std::optional<size_t> z_order)
{
    std::unique_lock lock(mScratcherMutex);
    if (z_order)
        mScratchers.emplace(mScratchers.begin() + *z_order, std::move(s));
    else
        mScratchers.emplace_back(std::move(s));
}

void DataScratchWidget::RemoveScratcher(const std::shared_ptr<Scratcher> &scratcher)
{
    std::unique_lock lock(mScratcherMutex);
    auto it = std::find(mScratchers.begin(), mScratchers.end(), scratcher);
    if (it != mScratchers.end()) mScratchers.erase(it);
}

namespace {

const double tick_margin_k = 0.25;
const int window_margin = 20;

}

void TimeRuler::BeforePaint(DataScratchWidget &w) const
{
    auto text_height = w.fontMetrics().height();
    const auto& r = w.GetClientRect();
    w.clientRect().setCoords(r.left(), r.top(), r.right(), w.rect().bottom() - text_height*2 - text_height/2 - 1);

    if (w.forceScale) {
        w.mDataViewRect.h = w.clientRect().height() * w.mYScale;
    }
    else {
        w.mYScale =  static_cast<double>(w.mDataViewRect.h) / w.clientRect().height();
    }
}

void TimeRuler::Paint(DataScratchWidget& w) const
{
    QDateTime start;
    start.setMSecsSinceEpoch(w.GetDataViewRect().x);

    auto window_msec = milliseconds(w.GetDataViewRect().w);
    auto window_sec_count = duration_cast<seconds>(window_msec).count();
    auto window_min_count = duration_cast<minutes>(window_msec).count();
    auto window_hour_count =duration_cast<hours>(window_msec).count();
    auto window_day_count = duration_cast<days>(window_msec).count();

    QPainter p(&w);
    QPen pen(Qt::gray, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
    p.setPen(pen);
    p.setBrush(Qt::gray);

    const QRect& rect = w.GetClientRect();
    auto text_height = p.fontMetrics().height();
    int axis_y = rect.bottom()+1;
    int tick_y2 = axis_y + text_height/2;
    int tick_y1 = tick_y2 - text_height;
    std::vector<uint64_t> ticks;

    if (window_day_count >=3) {
        ticks = w.GetTimeTicks<days>();
    }
    else if (window_hour_count >= 3) {
        ticks = w.GetTimeTicks<hours>();
    }
    else if (window_min_count >= 3) {
        ticks = w.GetTimeTicks<minutes>();
    }
    else if (window_sec_count >= 3) {
        ticks = w.GetTimeTicks<seconds>();
    }

    for (uint64_t tick: ticks) {
        int t = w.DataXToWidgetX(tick);
        p.drawLine(QPoint(t, tick_y1), QPoint(t, tick_y2));
    }

    p.drawLine(QPoint(rect.left(), axis_y), QPoint(rect.right(), axis_y));

    QString label(start.date().toString(w.locale().dateFormat(QLocale::ShortFormat)));

    p.drawText(QPoint{5, w.size().height() - 1}, label);
}

void PriceRuler::BeforePaint(DataScratchWidget &w) const
{
    currency<uint64_t> max_price = point;
    max_price.set_raw(w.GetDataViewRect().y_end());

    std::string label = max_price.to_string();
    for (auto& c: label) c = '0';

    int label_width = w.fontMetrics().horizontalAdvance(QString::fromLatin1(label));
    auto char_width = w.fontMetrics().horizontalAdvance('O');

    QRect r = w.GetClientRect();
    w.clientRect().setCoords(r.left(), r.top(), w.rect().right() - label_width - char_width*2 - 1, r.bottom());

    if (w.forceScale) {
        uint64_t old_end = w.mDataViewRect.x_end();

        w.mDataViewRect.w = w.clientRect().width() * w.mXScale;
        w.mDataViewRect.x = old_end - w.mDataViewRect.w;
    }
    else {
        w.mXScale = static_cast<double>(w.mDataViewRect.w) / w.clientRect().width();
    }
}

void PriceRuler::Paint(DataScratchWidget &w) const
{
    QPainter p(&w);
    QPen pen(Qt::gray, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
    p.setPen(pen);
    p.setBrush(Qt::gray);

    const QRect& rect = w.GetClientRect();
    auto text_height = p.fontMetrics().tightBoundingRect("0").height();
    auto char_width = p.fontMetrics().horizontalAdvance('O');
    int axis_x = rect.right() + 1;
    int tick_x1 = axis_x - char_width;
    int tick_x2 = axis_x + char_width;

    std::vector<uint64_t> ticks;

    uint64_t scale = 1;
    uint64_t steps = w.GetDataViewRect().h;

    std::clog << "DataView height: " << steps << std::endl;

    while (steps > 10) {
        steps /= 10;
        scale *= 10;
    }
    ticks = w.GetPriceTicks(scale);

    for (uint64_t tick: ticks) {
        int t = w.DataYToWidgetY(tick);
        p.drawLine(QPoint(tick_x1, t), QPoint(tick_x2, t));

        currency<uint64_t> tick_price = point;
        tick_price.set_raw(tick);

        std::string label = tick_price.to_string();
        p.drawText(QPoint(tick_x2 + char_width/2, t + text_height/2), QString::fromLatin1(label));
    }
    p.drawLine(QPoint(axis_x, rect.top()), QPoint(axis_x, rect.bottom()));
}

QRect DataScratchWidget::getQuotesArea() const
{
    QRect r(rect());
    return QRect(r.x() + window_margin, r.y() + window_margin, r.width() - window_margin*2, r.height() - window_margin * 2);
}


void DataScratchWidget::calculateResetTimeScale()
{
    // const static int def_tick_width = 10;
    // const QRect area = getQuotesArea();
    //
    // if (def_tick_width * m_quotes.size() > area.width()) {
    //     m_start = m_quotes.size() - (area.width() / def_tick_width);
    //     m_end = m_quotes.size() + 1;
    // }
    // else {
    //     m_start = 0;
    //     m_end = area.width() / def_tick_width;
    // }
}

void DataScratchWidget::calculateResetQuoteScale()
{
    // if (m_start == -1 || m_end == -1) throw std::runtime_error("No time scale calculated");
    //
    // double min_quote = std::numeric_limits<double>::max();
    // double max_quote = std::numeric_limits<double>::min();
    //
    // int range = std::ceil(m_end) - static_cast<size_t>(m_start);
    // auto end = (m_start + range) > m_quotes.size() ? m_quotes.end() : m_quotes.begin() + range;
    //
    // for (const auto& tick: std::ranges::subrange(m_quotes.begin() + static_cast<size_t>(m_start), end)) {
    //     min_quote = std::min(min_quote, std::min(tick[1], tick[2]));
    //     max_quote = std::max(max_quote, std::max(tick[1], tick[2]));
    // }
    //
    // m_min = min_quote;
    // m_max = max_quote;
}

void DataScratchWidget::drawQuoteChart(QPainter& painter)
{
    // size_t start_idx = std::floor(m_start);
    // size_t end_idx = std::ceil(m_end);
    //
    // auto area = getQuotesArea();
    // double tick_width = width() / (m_end - m_start);
    // double tick_offset_x = area.x() + (m_start - static_cast<double>(start_idx)) * tick_width;
    //
    // QPen redPen(Qt::red, 0, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
    // QPen greenPen(Qt::green, 0, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
    //
    // for(const auto [idx, tick]: std::views::zip(std::views::iota(start_idx, end_idx), m_quotes)) {
    //
    //     int y0 = area.x() + area.height() * (1 - (tick[0] - m_min) / (m_max - m_min));
    //     int y1 = area.x() + area.height() * (1 - (tick[1] - m_min) / (m_max - m_min));
    //     int y2 = area.x() + area.height() * (1 - (tick[2] - m_min) / (m_max - m_min));
    //     int y3 = area.x() + area.height() * (1 - (tick[3] - m_min) / (m_max - m_min));
    //
    //     int x0 = tick_offset_x;
    //     int x1 = tick_offset_x + (tick_width * (1 - tick_margin_k) / 2);
    //     int x2 = tick_offset_x + (tick_width * (1 - tick_margin_k));
    //
    //     painter.setPen(tick[0] <= tick[3] ? greenPen : redPen);
    //     painter.setBrush(tick[0] <= tick[3] ? Qt::green : Qt::red);
    //     painter.drawLine(QPoint(x1, y1), QPoint(x1, y2-1));
    //     painter.drawRect(QRect(QPoint(x0, std::min(y0, y3)), QPoint(x2-1, std::max(y0, y3)-1)));
    //
    //     tick_offset_x += tick_width;
    // }
}


void DataScratchWidget::paintEvent(QPaintEvent *event)
{
    mClientRect = rect();

    {
        std::shared_lock lock(mScratcherMutex);

        for (const auto &scr : mScratchers) {
            scr->BeforePaint(*this);
        }
        forceScale = false;

        for (const auto &scr : mScratchers) {
            scr->Paint(*this);
        }
    }

    QPainter p(this);
    QPen redPen(Qt::red, 5, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
    p.setPen(redPen);
    p.setBrush(Qt::red);

    p.drawLine(GetClientRect().topLeft(), GetClientRect().bottomRight());

}

void DataScratchWidget::resizeEvent(QResizeEvent *event)
{
    std::clog << "Resize <<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;

    forceScale = event->oldSize().width() > 0 && event->oldSize().height() > 0;

    QWidget::resizeEvent(event);
}

}
