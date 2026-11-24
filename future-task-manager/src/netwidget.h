#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPair>
#include <QObject>
#include "sysinfo.h"
#include "charthelper.h"

class NetWidget : public QWidget {
public:
    NetWidget(const NetInfo &info, QWidget *parent = nullptr)
        : QWidget(parent), netInfo(info) {
        prevNet = SysInfoReader::readNetStats(netInfo.iface);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(20, 18, 20, 18);

        auto *header = new QHBoxLayout();
        auto *title = new QLabel(QObject::tr("Network (%1)").arg(QString::fromStdString(netInfo.iface)));
        title->setStyleSheet("font-size: 24px; font-weight: bold;");
        auto *lblAdapter = new QLabel(QString::fromStdString(netInfo.adapterModel));
        lblAdapter->setStyleSheet("font-size: 13px; color: gray;");
        header->addWidget(title);
        header->addStretch();
        header->addWidget(lblAdapter);

        auto *bottomLayout = new QHBoxLayout();
        bottomLayout->setContentsMargins(0, 10, 0, 0);

        auto *leftGrid = new QGridLayout();
        leftGrid->setHorizontalSpacing(28);
        leftGrid->setVerticalSpacing(6);

        lblTx = makeBig(leftGrid, QObject::tr("Send"), "0 Kbps", 0, 0);
        lblRx = makeBig(leftGrid, QObject::tr("Receive"), "0 Kbps", 0, 1);

        auto *rightGrid = new QGridLayout();
        rightGrid->setHorizontalSpacing(18);
        rightGrid->setVerticalSpacing(3);

        QString stText = (netInfo.state == "Connected") ? QObject::tr("Connected") : QObject::tr("Disconnected");
        addSpec(rightGrid, 0, QObject::tr("Adapter:"), QString::fromStdString(netInfo.adapterModel));
        addSpec(rightGrid, 1, QObject::tr("Interface:"), QString::fromStdString(netInfo.iface));
        lblStatusVal = addSpec(rightGrid, 2, QObject::tr("Status:"), stText);

        bottomLayout->addLayout(leftGrid);
        bottomLayout->addStretch();
        bottomLayout->addLayout(rightGrid);

        layout->addLayout(header);
        layout->addWidget(ChartHelper::createAreaChart(series, QColor("#b7472a"), false), 1);
        layout->addLayout(bottomLayout);
    }

    QPair<QString, QString> updateData(double elapsedSec) {
        NetMetrics curr = SysInfoReader::readNetStats(netInfo.iface);
        QString rxStr = "0 Kbps", txStr = "0 Kbps";
        if (elapsedSec > 0.0) {
            double rxBits = (qMax(0LL, curr.rxBytes - prevNet.rxBytes) * 8.0) / elapsedSec;
            double txBits = (qMax(0LL, curr.txBytes - prevNet.txBytes) * 8.0) / elapsedSec;
            double totalBits = rxBits + txBits;

            if (totalBits > peakBits) peakBits = totalBits;
            lastPct = (peakBits > 0) ? (totalBits / peakBits) * 100.0 : 0.0;
            ChartHelper::pushValue(series, lastPct);

            rxStr = formatSpeed(rxBits);
            txStr = formatSpeed(txBits);
            lblRx->setText(rxStr);
            lblTx->setText(txStr);
        }
        prevNet = curr;
        return {rxStr, txStr};
    }

    QString ifaceName() const { return QString::fromStdString(netInfo.iface); }
    bool isConnected() const { return netInfo.state == "Connected"; }
    double lastPercent() const { return lastPct; }

private:
    NetInfo netInfo;
    NetMetrics prevNet;
    QLineSeries *series;
    QLabel *lblRx, *lblTx, *lblStatusVal;
    double peakBits = 100000.0;
    double lastPct = 0.0;

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

    static QString formatSpeed(double bitsPerSec) {
        if (bitsPerSec >= 1e9) return QString("%1 Gbps").arg(bitsPerSec / 1e9, 0, 'f', 1);
        if (bitsPerSec >= 1e6) return QString("%1 Mbps").arg(bitsPerSec / 1e6, 0, 'f', 1);
        return QString("%1 Kbps").arg(bitsPerSec / 1e3, 0, 'f', 0);
    }
};
