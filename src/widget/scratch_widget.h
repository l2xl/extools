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

#ifndef MARKETWIDGET_H
#define MARKETWIDGET_H

#include <deque>
#include <array>
#include <shared_mutex>

#include <QWidget>
#include <QPainter>
#include <QResizeEvent>

#include "currency.hpp"
#include "core_common.hpp"
#include "timedef.hpp"
#include "scratcher.hpp"
#include "quote_scratcher.hpp"

namespace scratcher {

class TimeRuler : public Scratcher
{
public:
    ~TimeRuler() override = default;
    void CalculateSize(DataScratchWidget &widget) override;
    void CalculatePaint(Rectangle &) override {}
    void Paint(DataScratchWidget &widget) const override;
};

class PriceRuler : public Scratcher
{
    const currency<uint64_t> m_point;
public:
    PriceRuler(currency<uint64_t> p) : m_point(p) {}
    ~PriceRuler() override = default;
    void CalculateSize(DataScratchWidget &widget) override;
    void CalculatePaint(Rectangle &) override {}
    void Paint(DataScratchWidget &widget) const override;
};

class PriceIndicator : public Scratcher
{
    const currency<uint64_t> m_point;
    std::shared_ptr<const IDataProvider> mDataManager;
public:
    PriceIndicator(std::shared_ptr<const IDataProvider> dataManager) : m_point(dataManager->PricePoint()), mDataManager(std::move(dataManager)) {}
    ~PriceIndicator() override = default;
    void CalculateSize(DataScratchWidget &widget) override {}
    void CalculatePaint(Rectangle &) override {}
    void Paint(DataScratchWidget &widget) const override;
};

class Margin : public Scratcher
{
    double m_margin_rate;
public:
    Margin(double margin_rate) : m_margin_rate(margin_rate) {}
    ~Margin() override = default;
    void CalculateSize(DataScratchWidget &widget) override {}
    void CalculatePaint(Rectangle &) override;
    void Paint(DataScratchWidget &widget) const override {}
};

class DataScratchWidget : public QWidget
{
    friend class TimeRuler;
    friend class PriceRuler;
    friend class PriceIndicator;
    friend class QuoteScratcher;

    Q_OBJECT

    Rectangle mDataViewRect;
    QRect mClientRect {0,0,0,0};

    double mXScale = 1.0;
    double mYScale = 1.0;

    std::list<std::function<void(DataScratchWidget& )>> m_data_view_listeners;

    std::shared_ptr<QuoteScratcher> mQuoteScratcher;
    std::deque<std::shared_ptr<Scratcher>> mScratchers;
    std::shared_mutex mScratcherMutex;

    QRect& clientRect() { return mClientRect; }
    const std::shared_ptr<QuoteScratcher>& quoteScratcher() { return mQuoteScratcher; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

public slots:
    void DataViewRectChanged(Rectangle rect);

public:
    explicit DataScratchWidget(QWidget *parent = nullptr);

    const Rectangle& GetDataViewRect() const { return mDataViewRect; }
    const QRect& GetClientRect() const { return mClientRect; }

    void AddDataViewChangeListener(std::function<void(DataScratchWidget& w)> handler)
    { m_data_view_listeners.emplace_back(handler); }

    int DataXToWidgetX(uint64_t x) const;
    int DataYToWidgetY(uint64_t x) const;

    void AddQuoteScratcher(std::shared_ptr<QuoteScratcher> s) { AddScratcher(mQuoteScratcher = std::move(s), {0}); }
    void AddScratcher(std::shared_ptr<Scratcher> scratcher, std::optional<size_t> z_order = {});
    void RemoveScratcher(const std::shared_ptr<Scratcher> &scratcher);

    const std::deque<std::shared_ptr<Scratcher>>& Scratchers() const
    { return mScratchers; }

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
        if (GetDataViewRect().y_start() != std::numeric_limits<uint64_t>::max()) {
            for (int tick = ((GetDataViewRect().y / step) + 1) * step; tick < GetDataViewRect().y_end(); tick += step) {
                ticks.push_back(tick);
            }
        }
        return ticks;
    }
};

}

#endif // MARKETWIDGET_H
