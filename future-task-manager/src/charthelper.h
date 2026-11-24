#pragma once
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QValueAxis>

class ChartHelper {
public:
    static QChartView* createAreaChart(QLineSeries *&series, const QColor &color, bool compact = false) {
        series = new QLineSeries();
        for (int i = 0; i < 60; ++i) series->append(i, 0);

        auto *area = new QAreaSeries(series);
        QPen pen(color);
        pen.setWidth(compact ? 1 : 2);
        area->setPen(pen);
        QColor fill = color;
        fill.setAlpha(45);
        area->setBrush(fill);

        auto *chart = new QChart();
        chart->addSeries(area);
        chart->legend()->hide();
        chart->setBackgroundVisible(false);
        chart->setMargins(compact ? QMargins(2, 2, 2, 2) : QMargins(5, 5, 5, 5));

        auto *axisX = new QValueAxis();
        axisX->setRange(0, 59);
        axisX->setLabelsVisible(false);
        axisX->setGridLineColor(QColor(128, 128, 128, 35));

        auto *axisY = new QValueAxis();
        axisY->setRange(0, 100);
        axisY->setLabelsVisible(false);
        axisY->setGridLineColor(QColor(128, 128, 128, 35));

        chart->addAxis(axisX, Qt::AlignBottom);
        chart->addAxis(axisY, Qt::AlignLeft);
        area->attachAxis(axisX);
        area->attachAxis(axisY);

        auto *view = new QChartView(chart);
        view->setRenderHint(QPainter::Antialiasing);
        return view;
    }

    static void pushValue(QLineSeries *series, double val) {
        if (!series) return;
        QList<QPointF> pts = series->points();
        for (int i = 0; i < pts.size() - 1; ++i) pts[i].setY(pts[i + 1].y());
        pts.last().setY(qBound(0.0, val, 100.0));
        series->replace(pts);
    }
};
