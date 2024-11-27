#ifndef MARKETWIDGET_H
#define MARKETWIDGET_H

#include <deque>
#include <array>

#include <QWidget>

class MarketWidget : public QWidget
{
    Q_OBJECT

    double m_start = -1;
    double m_end = -1;

    double m_min;
    double m_max;

    std::deque<std::array<double, 4>> m_quotes;


    QRect getQuotesArea() const;

    void calculateResetTimeScale();
    void calculateResetQuoteScale();
    void drawQuoteChart(QPainter& painter);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
public:
    explicit MarketWidget(QWidget *parent = nullptr);

    template <typename C>
    void SetMarketData(C&& newdata)
    { m_quotes = std::forward<C>(newdata); update(); }

    template <typename C>
    void AppendMarketData(const C& newdata)
    { for(const auto& item: newdata) m_quotes.emplace_back(item); update();}

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

#endif // MARKETWIDGET_H
