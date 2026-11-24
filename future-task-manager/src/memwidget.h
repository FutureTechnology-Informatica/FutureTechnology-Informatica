#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QObject>
#include "sysinfo.h"
#include "charthelper.h"

class MemWidget : public QWidget {
public:
    MemWidget(QWidget *parent = nullptr) : QWidget(parent) {
        MemMetrics initM = SysInfoReader::readMemory();

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(20, 18, 20, 18);

        auto *header = new QHBoxLayout();
        auto *title = new QLabel(QObject::tr("Memory"));
        title->setStyleSheet("font-size: 24px; font-weight: bold;");
        lblTotal = new QLabel(QString("%1 GB").arg(initM.totalGb, 0, 'f', 1));
        lblTotal->setStyleSheet("font-size: 16px; color: gray;");
        header->addWidget(title);
        header->addStretch();
        header->addWidget(lblTotal);

        auto *bottomLayout = new QHBoxLayout();
        bottomLayout->setContentsMargins(0, 10, 0, 0);

        auto *leftGrid = new QGridLayout();
        leftGrid->setHorizontalSpacing(28);
        leftGrid->setVerticalSpacing(6);

        lblUsed      = makeBig(leftGrid, QObject::tr("In use"), "0.0 GB", 0, 0);
        lblAvail     = makeBig(leftGrid, QObject::tr("Available"), "0.0 GB", 0, 1);
        lblCommitted = makeBig(leftGrid, QObject::tr("Committed"), "0.0 GB", 1, 0);
        lblCached    = makeBig(leftGrid, QObject::tr("Cached (RAM)"), "0.0 GB", 1, 1);

        auto *rightGrid = new QGridLayout();
        rightGrid->setHorizontalSpacing(18);
        rightGrid->setVerticalSpacing(3);

        lblSwapVal = addSpec(rightGrid, 0, QObject::tr("Swap in use:"), "0.0 / 0.0 GB");
        addSpec(rightGrid, 1, "Cache L1 (CPU):", QString::fromStdString(initM.cacheL1));
        addSpec(rightGrid, 2, "Cache L2 (CPU):", QString::fromStdString(initM.cacheL2));
        addSpec(rightGrid, 3, "Cache L3 (CPU):", QString::fromStdString(initM.cacheL3));

        bottomLayout->addLayout(leftGrid);
        bottomLayout->addStretch();
        bottomLayout->addLayout(rightGrid);

        layout->addLayout(header);
        layout->addWidget(ChartHelper::createAreaChart(series, QColor("#8b12ae"), false), 1);
        layout->addLayout(bottomLayout);
    }

    MemMetrics updateData() {
        MemMetrics m = SysInfoReader::readMemory();
        ChartHelper::pushValue(series, m.percent);
        lblTotal->setText(QString("%1 GB").arg(m.totalGb, 0, 'f', 1));
        lblUsed->setText(QString("%1 GB").arg(m.usedGb, 0, 'f', 1));
        lblAvail->setText(QString("%1 GB").arg(m.availGb, 0, 'f', 1));
        lblCommitted->setText(QString("%1 GB").arg(m.committedGb, 0, 'f', 1));
        lblCached->setText(QString("%1 GB").arg(m.cachedGb, 0, 'f', 1));
        lblSwapVal->setText(QString("%1 / %2 GB").arg(m.swapUsedGb, 0, 'f', 1).arg(m.swapTotalGb, 0, 'f', 1));
        return m;
    }

private:
    QLineSeries *series;
    QLabel *lblTotal, *lblUsed, *lblAvail, *lblCommitted, *lblCached, *lblSwapVal;

    QLabel* makeBig(QGridLayout *g, const QString &cap, const QString &val, int r, int c) {
        auto *box = new QVBoxLayout();
        box->setSpacing(1);
        auto *lc = new QLabel(cap); lc->setStyleSheet("font-size: 11px; color: gray;");
        auto *lv = new QLabel(val); lv->setStyleSheet("font-size: 18px; font-weight: 500;");
        box->addWidget(lc); box->addWidget(lv);
        g->addLayout(box, r, c);
        return lv;
    }

    QLabel* addSpec(QGridLayout *g, int row, const QString &k, const QString &v) {
        auto *lk = new QLabel(k); lk->setStyleSheet("font-size: 12px; color: gray;");
        auto *lv = new QLabel(v); lv->setStyleSheet("font-size: 12px; font-weight: 500;");
        g->addWidget(lk, row, 0); g->addWidget(lv, row, 1);
        return lv;
    }
};
