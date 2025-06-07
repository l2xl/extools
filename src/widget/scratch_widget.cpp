// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the Intellectual Property Reserve License (IPRL)
// -----BEGIN PGP PUBLIC KEY BLOCK-----
//
// mDMEYdxcVRYJKwYBBAHaRw8BAQdAfacBVThCP5QDPEgSbSIudtpJS4Y4Imm5dzaN
// lM1HTem0IkwyIFhsIChsMnhsKSA8bDJ4bEBwcm90b25tYWlsLmNvbT6IkAQTFggA
// OBYhBKRCfUyWnduCkisNl+WRcOaCK79JBQJh3FxVAhsDBQsJCAcCBhUKCQgLAgQW
// AgMBAh4BAheAAAoJEOWRcOaCK79JDl8A/0/AjYVbAURZJXP3tHRgZyYyN9txT6mW
// 0bYCcOf0rZ4NAQDoFX4dytPDvcjV7ovSQJ6dzvIoaRbKWGbHRCufrm5QBA==
// =KKu7
// -----END PGP PUBLIC KEY BLOCK-----

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
{
    // Ensure this widget uses the application's palette
    setPalette(QApplication::palette());
    
    // Make sure this widget doesn't have a custom background
    setAutoFillBackground(true);
}

void DataScratchWidget::SetDataViewRect(const Rectangle& rect)
{
    std::clog << "SetDataViewRect ^^^^^^^^^^^^" << std::endl;


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

void TimeRuler::Resize(DataScratchWidget &w)
{
    auto text_height = w.fontMetrics().height();
    const auto& r = w.GetClientRect();
    w.clientRect().setCoords(r.left(), r.top(), r.right(), w.rect().bottom() - text_height*2 - text_height/2 - 1);
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

void PriceRuler::Resize(DataScratchWidget &w)
{
    currency<uint64_t> max_price = point;
    max_price.set_raw(w.GetDataViewRect().y_end());

    std::string label = max_price.to_string();
    for (auto& c: label) c = '0';

    int label_width = w.fontMetrics().horizontalAdvance(QString::fromLatin1(label));
    auto char_width = w.fontMetrics().horizontalAdvance('O');

    QRect r = w.GetClientRect();
    w.clientRect().setCoords(r.left(), r.top(), w.rect().right() - label_width - char_width*2 - 1, r.bottom());
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

void DataScratchWidget::paintEvent(QPaintEvent *event)
{
    std::clog << "Paint event **********" << std::endl;
    {
        std::shared_lock lock(mScratcherMutex);

        for (const auto &scr : mScratchers) {
            scr->BeforePaint(*this);
        }

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

    std::clog << "Resize event ==========" << std::endl;

    // Scratchers must have chance to adjust client rectangle if they needed some space out of quote graph
    mClientRect = rect();

    {
        std::shared_lock lock(mScratcherMutex);

        for (const auto &s : mScratchers) {
            s->Resize(*this);
        }
    }

    if (event->oldSize().width() > 0 && event->oldSize().height() > 0) {
        uint64_t old_end = mDataViewRect.x_end();
        mDataViewRect.h = clientRect().height() * mYScale;
        mDataViewRect.w = clientRect().width() * mXScale;
        mDataViewRect.x = old_end - mDataViewRect.w;
    }
    else {
        std::clog << "Resize from zero" << std::endl;
        if (mDataViewRect.w) mXScale = static_cast<double>(mDataViewRect.w) / clientRect().width();
        if (mDataViewRect.h) mYScale = static_cast<double>(mDataViewRect.h) / clientRect().height();
    }

    for (const auto& h: m_data_view_listeners) {
        h(*this);
    }

    QWidget::resizeEvent(event);
}

}
