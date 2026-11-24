#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QMenu>
#include <QActionGroup>
#include <QClipboard>
#include <QApplication>
#include <QSettings>
#include <QObject>
#include <cmath>
#include "sysinfo.h"
#include "charthelper.h"

class CpuWidget : public QWidget {
public:
    CpuWidget(QWidget *parent = nullptr) : QWidget(parent) {
        QSettings settings("FutureTechnology", "FutureTaskManager");
        showLogical = settings.value("showLogicalProcessors", false).toBool();
        CpuDetails initD = SysInfoReader::readCpuDetails();

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(20, 18, 20, 18);

        auto *header = new QHBoxLayout();
        auto *title = new QLabel("CPU");
        title->setStyleSheet("font-size: 24px; font-weight: bold;");
        lblModel = new QLabel(QString::fromStdString(initD.model));
        lblModel->setStyleSheet("font-size: 13px; color: gray;");
        header->addWidget(title);
        header->addStretch();
        header->addWidget(lblModel);

        auto *subHeader = new QHBoxLayout();
        auto *lblSub = new QLabel(QObject::tr("% Utilization"));
        lblSub->setStyleSheet("font-size: 11px; color: gray;");
        lblOs = new QLabel(QString::fromStdString(SysInfoReader::getOsAndKernel()));
        lblOs->setStyleSheet("font-size: 11px; color: #3daee9; font-weight: bold;");
        subHeader->addWidget(lblSub);
        subHeader->addStretch();
        subHeader->addWidget(lblOs);

        chartStack = new QStackedWidget();
        auto *overallView = ChartHelper::createAreaChart(overallSeries, QColor("#117dbb"), false);
        attachContextMenu(overallView);
        chartStack->addWidget(overallView);

        auto *logicalBox = new QWidget();
        auto *grid = new QGridLayout(logicalBox);
        grid->setSpacing(6);
        grid->setContentsMargins(0, 4, 0, 4);

        prevTimes = SysInfoReader::readCpu();
        int threads = qMax(1, static_cast<int>(prevTimes.size()) - 1);
        int cols = static_cast<int>(std::ceil(std::sqrt(threads)));

        for (int i = 0; i < threads; ++i) {
            QLineSeries *s = nullptr;
            auto *v = ChartHelper::createAreaChart(s, QColor("#117dbb"), true);
            attachContextMenu(v);
            logicalSeries.append(s);
            grid->addWidget(v, i / cols, i % cols);
        }
        chartStack->addWidget(logicalBox);
        chartStack->setCurrentIndex(showLogical ? 1 : 0);

        auto *bottomLayout = new QHBoxLayout();
        bottomLayout->setContentsMargins(0, 10, 0, 0);

        auto *leftGrid = new QGridLayout();
        leftGrid->setHorizontalSpacing(28);
        leftGrid->setVerticalSpacing(6);

        lblUtilVal   = makeBigMetric(leftGrid, QObject::tr("Utilization"), "0%", 0, 0);
        lblSpeedVal  = makeBigMetric(leftGrid, QObject::tr("Speed"), "0.00 GHz", 0, 1);
        lblProcVal   = makeBigMetric(leftGrid, QObject::tr("Processes"), "0", 1, 0);
        lblThreadVal = makeBigMetric(leftGrid, QObject::tr("Threads"), "0", 1, 1);
        lblUptimeVal = makeBigMetric(leftGrid, QObject::tr("Up time"), "0:00:00:00", 2, 0);

        auto *rightGrid = new QGridLayout();
        rightGrid->setHorizontalSpacing(18);
        rightGrid->setVerticalSpacing(3);

        QString virtText = (initD.virtualization == "Enabled")
            ? QObject::tr("Enabled") : QObject::tr("Disabled");

        addSpecRow(rightGrid, 0, QObject::tr("Maximum speed:"), QString("%1 GHz").arg(initD.maxGhz, 0, 'f', 2));
        addSpecRow(rightGrid, 1, QObject::tr("Sockets:"), QString::number(initD.sockets));
        addSpecRow(rightGrid, 2, QObject::tr("Cores:"), QString::number(initD.physicalCores));
        addSpecRow(rightGrid, 3, QObject::tr("Logical processors:"), QString::number(initD.logicalProcessors));
        addSpecRow(rightGrid, 4, QObject::tr("Virtualization:"), virtText);
        addSpecRow(rightGrid, 5, "Cache L1:", QString::fromStdString(initD.cacheL1));
        addSpecRow(rightGrid, 6, "Cache L2:", QString::fromStdString(initD.cacheL2));
        addSpecRow(rightGrid, 7, "Cache L3:", QString::fromStdString(initD.cacheL3));

        bottomLayout->addLayout(leftGrid);
        bottomLayout->addStretch();
        bottomLayout->addLayout(rightGrid);

        layout->addLayout(header);
        layout->addLayout(subHeader);
        layout->addWidget(chartStack, 1);
        layout->addLayout(bottomLayout);
    }

    CpuDetails updateData(double &outPct) {
        auto curr = SysInfoReader::readCpu();
        outPct = 0.0;
        if (!curr.empty() && curr.size() == prevTimes.size()) {
            long long totDiff = curr[0].total - prevTimes[0].total;
            long long idlDiff = curr[0].idle - prevTimes[0].idle;
            outPct = totDiff > 0 ? (100.0 * (totDiff - idlDiff) / totDiff) : 0.0;
            ChartHelper::pushValue(overallSeries, outPct);

            for (size_t i = 1; i < curr.size() && (i - 1) < static_cast<size_t>(logicalSeries.size()); ++i) {
                long long cTot = curr[i].total - prevTimes[i].total;
                long long cIdl = curr[i].idle - prevTimes[i].idle;
                double cPct = cTot > 0 ? (100.0 * (cTot - cIdl) / cTot) : 0.0;
                ChartHelper::pushValue(logicalSeries[i - 1], cPct);
            }
        }
        prevTimes = curr;

        CpuDetails d = SysInfoReader::readCpuDetails();
        lblUtilVal->setText(QString("%1%").arg(outPct, 0, 'f', 0));
        lblSpeedVal->setText(QString("%1 GHz").arg(d.currentGhz, 0, 'f', 2));
        lblProcVal->setText(QString::number(d.processes));
        lblThreadVal->setText(QString::number(d.threads));
        lblUptimeVal->setText(QString::fromStdString(d.uptime));
        return d;
    }

private:
    bool showLogical = false;
    QStackedWidget *chartStack;
    QLineSeries *overallSeries;
    QVector<QLineSeries*> logicalSeries;
    QLabel *lblModel, *lblOs;
    QLabel *lblUtilVal, *lblSpeedVal, *lblProcVal, *lblThreadVal, *lblUptimeVal;
    std::vector<CpuTimes> prevTimes;

    QLabel* makeBigMetric(QGridLayout *grid, const QString &caption, const QString &initVal, int r, int c) {
        auto *box = new QVBoxLayout();
        box->setSpacing(1);
        auto *lblCap = new QLabel(caption);
        lblCap->setStyleSheet("font-size: 11px; color: gray;");
        auto *lblVal = new QLabel(initVal);
        lblVal->setStyleSheet("font-size: 18px; font-weight: 500;");
        box->addWidget(lblCap);
        box->addWidget(lblVal);
        grid->addLayout(box, r, c);
        return lblVal;
    }

    void addSpecRow(QGridLayout *grid, int row, const QString &key, const QString &val) {
        auto *k = new QLabel(key);
        k->setStyleSheet("font-size: 12px; color: gray;");
        auto *v = new QLabel(val);
        v->setStyleSheet("font-size: 12px; font-weight: 500;");
        grid->addWidget(k, row, 0);
        grid->addWidget(v, row, 1);
    }

    void attachContextMenu(QWidget *target) {
        target->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(target, &QWidget::customContextMenuRequested, this, [this, target](const QPoint &pos) {
            QMenu menu(this);
            QMenu *sub = menu.addMenu(QObject::tr("Change graph to"));
            auto *grp = new QActionGroup(&menu);
            auto *actAll = sub->addAction(QObject::tr("Overall utilization"));
            actAll->setCheckable(true); actAll->setChecked(!showLogical); grp->addAction(actAll);
            auto *actLog = sub->addAction(QObject::tr("Logical processors"));
            actLog->setCheckable(true); actLog->setChecked(showLogical); grp->addAction(actLog);

            menu.addSeparator();
            auto *actCopy = menu.addAction(QObject::tr("Copy info"));

            QAction *sel = menu.exec(target->mapToGlobal(pos));
            if (sel == actAll || sel == actLog) {
                showLogical = (sel == actLog);
                chartStack->setCurrentIndex(showLogical ? 1 : 0);
                QSettings("FutureTechnology", "FutureTaskManager").setValue("showLogicalProcessors", showLogical);
            } else if (sel == actCopy) {
                QApplication::clipboard()->setText(
                    QString("%1\n%2\n%3 | %4")
                        .arg(lblModel->text(), lblOs->text(), lblUtilVal->text(), lblSpeedVal->text()));
            }
        });
    }
};
