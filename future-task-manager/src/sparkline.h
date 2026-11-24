#pragma once
#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QVector>

class SparklineWidget : public QWidget {
public:
    SparklineWidget(const QColor &color, QWidget *parent = nullptr)
        : QWidget(parent), lineColor(color), values(40, 0.0) {
        setFixedSize(64, 42);
    }

    void pushValue(double val) {
        values.removeFirst();
        values.append(qBound(0.0, val, 100.0));
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QRectF r = rect().adjusted(1, 1, -1, -1);
        p.setPen(QPen(lineColor, 1.2));
        p.setBrush(QColor(lineColor.red(), lineColor.green(), lineColor.blue(), 15));
        p.drawRect(r);

        if (values.size() < 2) return;

        QPainterPath path;
        double stepX = r.width() / (values.size() - 1);
        for (int i = 0; i < values.size(); ++i) {
            double x = r.left() + i * stepX;
            double y = r.bottom() - (values[i] / 100.0) * r.height();
            if (i == 0) path.moveTo(x, y);
            else path.lineTo(x, y);
        }

        QPainterPath fillPath = path;
        fillPath.lineTo(r.right(), r.bottom());
        fillPath.lineTo(r.left(), r.bottom());
        fillPath.closeSubpath();

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(lineColor.red(), lineColor.green(), lineColor.blue(), 45));
        p.drawPath(fillPath);

        p.setPen(QPen(lineColor, 1.4));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
    }

private:
    QColor lineColor;
    QVector<double> values;
};
