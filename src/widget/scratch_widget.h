// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#ifndef MARKETWIDGET_H
#define MARKETWIDGET_H

#include <deque>
#include <array>
#include <deque>
#include <shared_mutex>

#include <QWidget>
#include <QPainter>
#include <QResizeEvent>

#include "currency.hpp"
#include "timedef.hpp"
#include "scratcher.hpp"

namespace scratcher {


struct Rectangle
{
    uint64_t x = 0, y = 0, w = 0, h = 0;
    uint64_t x_start() const { return x; }
    uint64_t x_end() const { return x + w; }
    uint64_t y_start() const { return y; }
    uint64_t y_end() const { return y + h; }
};

class TimeRuler : public Scratcher
{
public:
    ~TimeRuler() override = default;
    void BeforePaint(DataScratchWidget &widget) const;
    void Paint(DataScratchWidget &widget) const;
};

class PriceRuler : public Scratcher
{
    currency<uint64_t> point;
public:
    PriceRuler(currency<uint64_t> p) : point(p) {}
    ~PriceRuler() override = default;
    void BeforePaint(DataScratchWidget &widget) const;
    void Paint(DataScratchWidget &widget) const;
};

class DataScratchWidget : public QWidget
{
    friend class TimeRuler;
    friend class PriceRuler;
    friend class QuoteScratcher;

    Q_OBJECT

    // double m_start = -1;
    // double m_end = -1;
    //
    // double m_min;
    // double m_max;

    //Rectangle mDataField;
    Rectangle mDataViewRect;
    QRect mClientRect;

    bool forceScale = false;
    double mXScale = 1.0;
    double mYScale = 1.0;

    std::deque<std::shared_ptr<Scratcher>> mScratchers;
    std::shared_mutex mScratcherMutex;

    std::deque<std::array<double, 4>> m_quotes;

    QRect getQuotesArea() const;

    void calculateResetTimeScale();
    void calculateResetQuoteScale();
    void drawQuoteChart(QPainter& painter);

    QRect& clientRect() { return mClientRect; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
public:
    explicit DataScratchWidget(QWidget *parent = nullptr);

    //void SetDataFieldRect(const Rectangle& rect);

    //const Rectangle& GetDataFieldRect() const { return mDataField; }

    void SetDataViewRect(const Rectangle& rect);

    const Rectangle& GetDataViewRect() const { return mDataViewRect; }

    const QRect& GetClientRect() const { return mClientRect; }

    int DataXToWidgetX(uint64_t x) const;
    int DataYToWidgetY(uint64_t x) const;

    void AddScratcher(std::shared_ptr<Scratcher> scratcher, std::optional<size_t> z_order = {});
    void RemoveScratcher(const std::shared_ptr<Scratcher> &scratcher);

    template<typename PERIOD>
    std::vector<uint64_t> GetTimeTicks() const
    {
        std::vector<uint64_t> ticks;

        auto start = std::chrono::duration_cast<PERIOD>(std::chrono::milliseconds(GetDataViewRect().x) + PERIOD(1) - std::chrono::seconds(1)).count();
        for (auto tick = std::chrono::duration_cast<std::chrono::milliseconds>(PERIOD(start)).count();
            tick < GetDataViewRect().x_end();
            tick += std::chrono::duration_cast<std::chrono::milliseconds>(PERIOD(1)).count()) {

            ticks.push_back(tick);
        }
        return ticks;
    }

    std::vector<uint64_t> GetPriceTicks(uint64_t step) const
    {
        std::vector<uint64_t> ticks;
        for (int tick = ((GetDataViewRect().y / step) + 1) * step; tick < GetDataViewRect().y_end(); tick += step) {
            ticks.push_back(tick);
        }
        return ticks;
    }





    template <typename C>
    void SetMarketData(C&& newdata)
    {
        m_quotes = std::forward<C>(newdata);
        update();
    }

    template <typename C>
    void AppendMarketData(const C& newdata)
    {
        for(const auto& item: newdata) m_quotes.emplace_back(item);
        update();
    }

    void ResetTimeScale()
    {
        calculateResetTimeScale();
        if (isVisible())
            update();
    }
    void ResetQuoteScale()
    {
        calculateResetQuoteScale();
        if (isVisible())
            update();
    }
    void ResetScale()
    {
        calculateResetTimeScale();
        calculateResetQuoteScale();
        if (isVisible())
            update();
    }

    signals:
    };

}

#endif // MARKETWIDGET_H
