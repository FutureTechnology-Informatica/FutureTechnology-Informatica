#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QMenu>
#include <QActionGroup>
#include <QSettings>
#include <QObject>
#include <QVector>
#include <cmath>
#include "sysinfo.h"
#include "charthelper.h"

class DiskWidget : public QWidget {
public:
    DiskWidget(int index, const DiskInfo &info, QWidget *parent = nullptr)
        : QWidget(parent), diskInfo(info) {
        QSettings settings("FutureTechnology", "FutureTaskManager");
        QString key = "showPartitions_" + QString::fromStdString(diskInfo.device);
        showPartitions = settings.value(key, false).toBool();

        prevDisk = SysInfoReader::readDiskStats(diskInfo.device);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(20, 18, 20, 18);

        auto *header = new QHBoxLayout();
        auto *title = new QLabel(QObject::tr("Disk %1 (%2)").arg(index).arg(QString::fromStdString(diskInfo.device)));
        title->setStyleSheet("font-size: 24px; font-weight: bold;");
        auto *lblModel = new QLabel(QString::fromStdString(diskInfo.model));
        lblModel->setStyleSheet("font-size: 14px; color: gray;");
        header->addWidget(title);
        header->addStretch();
        header->addWidget(lblModel);

        chartStack = new QStackedWidget();

        auto *wholeView = ChartHelper::createAreaChart(series, QColor("#2e8b57"), false);
        attachContextMenu(wholeView);
        chartStack->addWidget(wholeView);

        auto *partBox = new QWidget();
        auto *pGrid = new QGridLayout(partBox);
        pGrid->setSpacing(8);
        pGrid->setContentsMargins(0, 4, 0, 4);

        int count = static_cast<int>(diskInfo.partList.size());
        int cols = count > 1 ? static_cast<int>(std::ceil(std::sqrt(count))) : 1;

        for (int i = 0; i < count; ++i) {
            const auto &pt = diskInfo.partList[i];
            prevPartStats.append(SysInfoReader::readDiskStats(pt.name));
            partPeaks.append(1.0);

            auto *cell = new QWidget();
            auto *cellLay = new QVBoxLayout(cell);
            cellLay->setContentsMargins(0, 0, 0, 0);
            cellLay->setSpacing(2);

            QString baseCap = QString::fromStdString(pt.name);
            if (!pt.mountPoint.empty()) baseCap += " (" + QString::fromStdString(pt.mountPoint) + ")";
            baseCap += QString(" — %1 GB").arg(pt.sizeGb, 0, 'f', 1);
            partBaseCaptions.append(baseCap);

            auto *lblCap = new QLabel(baseCap);
            lblCap->setStyleSheet("font-size: 11px; color: gray; font-weight: 500;");
            partLabels.append(lblCap);

            QLineSeries *ps = nullptr;
            auto *pv = ChartHelper::createAreaChart(ps, QColor("#2e8b57"), true);
            attachContextMenu(pv);
            partSeries.append(ps);

            cellLay->addWidget(lblCap);
            cellLay->addWidget(pv, 1);
            pGrid->addWidget(cell, i / cols, i % cols);
        }

        if (count == 0) {
            auto *emptyLbl = new QLabel("-");
            emptyLbl->setAlignment(Qt::AlignCenter);
            attachContextMenu(emptyLbl);
            pGrid->addWidget(emptyLbl, 0, 0);
        }

        chartStack->addWidget(partBox);
        chartStack->setCurrentIndex((showPartitions && count > 0) ? 1 : 0);

        auto *bottomLayout = new QHBoxLayout();
        bottomLayout->setContentsMargins(0, 10, 0, 0);

        auto *leftGrid = new QGridLayout();
        leftGrid->setHorizontalSpacing(28);
        leftGrid->setVerticalSpacing(6);

        lblActive = makeBig(leftGrid, QObject::tr("Active time"), "0%", 0, 0);
        lblRead   = makeBig(leftGrid, QObject::tr("Read speed"), "0.0 MB/s", 1, 0);
        lblWrite  = makeBig(leftGrid, QObject::tr("Write speed"), "0.0 MB/s", 1, 1);

        auto *rightGrid = new QGridLayout();
        rightGrid->setHorizontalSpacing(18);
        rightGrid->setVerticalSpacing(3);

        addSpec(rightGrid, 0, QObject::tr("Model:"), QString::fromStdString(diskInfo.model));
        addSpec(rightGrid, 1, QObject::tr("Capacity:"), QString("%1 GB").arg(diskInfo.sizeGb, 0, 'f', 1));
        addSpec(rightGrid, 2, QObject::tr("Device:"), "/dev/" + QString::fromStdString(diskInfo.device));
        addSpec(rightGrid, 3, QObject::tr("Partitions:"), QString::fromStdString(diskInfo.partitions));

        bottomLayout->addLayout(leftGrid);
        bottomLayout->addStretch();
        bottomLayout->addLayout(rightGrid);

        layout->addLayout(header);
        layout->addWidget(chartStack, 1);
        layout->addLayout(bottomLayout);
    }

    double updateData(double elapsedSec) {
        DiskMetrics curr = SysInfoReader::readDiskStats(diskInfo.device);
        double activePct = 0.0;
        if (elapsedSec > 0.0) {
            long long dTicks = qMax(0LL, curr.ioTicks - prevDisk.ioTicks);
            activePct = qBound(0.0, (dTicks / (elapsedSec * 1000.0)) * 100.0, 100.0);

            double readMb = qMax(0LL, curr.readBytes - prevDisk.readBytes) / (1048576.0 * elapsedSec);
            double writeMb = qMax(0LL, curr.writeBytes - prevDisk.writeBytes) / (1048576.0 * elapsedSec);

            ChartHelper::pushValue(series, activePct);
            lblActive->setText(QString("%1%").arg(activePct, 0, 'f', 0));
            lblRead->setText(QString("%1 MB/s").arg(readMb, 0, 'f', 1));
            lblWrite->setText(QString("%1 MB/s").arg(writeMb, 0, 'f', 1));

            for (int i = 0; i < partSeries.size() && i < static_cast<int>(diskInfo.partList.size()); ++i) {
                DiskMetrics pCurr = SysInfoReader::readDiskStats(diskInfo.partList[i].name);
                double pReadMb  = qMax(0LL, pCurr.readBytes - prevPartStats[i].readBytes) / (1048576.0 * elapsedSec);
                double pWriteMb = qMax(0LL, pCurr.writeBytes - prevPartStats[i].writeBytes) / (1048576.0 * elapsedSec);
                double totalMb  = pReadMb + pWriteMb;

                if (totalMb > partPeaks[i]) partPeaks[i] = totalMb;
                double pPct = (partPeaks[i] > 0.0) ? qBound(0.0, (totalMb / partPeaks[i]) * 100.0, 100.0) : 0.0;

                ChartHelper::pushValue(partSeries[i], pPct);
                partLabels[i]->setText(QString("%1  |  R: %2 MB/s  W: %3 MB/s")
                                       .arg(partBaseCaptions[i])
                                       .arg(pReadMb, 0, 'f', 1)
                                       .arg(pWriteMb, 0, 'f', 1));
                prevPartStats[i] = pCurr;
            }
        }
        prevDisk = curr;
        return activePct;
    }

    QString deviceName() const { return QString::fromStdString(diskInfo.device); }

private:
    DiskInfo diskInfo;
    DiskMetrics prevDisk;
    QVector<DiskMetrics> prevPartStats;
    QVector<double> partPeaks;
    QVector<QString> partBaseCaptions;
    QVector<QLabel*> partLabels;
    QStackedWidget *chartStack;
    QLineSeries *series;
    QVector<QLineSeries*> partSeries;
    QLabel *lblActive, *lblRead, *lblWrite;
    bool showPartitions = false;

    void attachContextMenu(QWidget *target) {
        target->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(target, &QWidget::customContextMenuRequested, this, [this, target](const QPoint &pos) {
            QMenu menu(this);
            QMenu *sub = menu.addMenu(QObject::tr("Change graph to"));
            auto *grp = new QActionGroup(&menu);

            auto *actWhole = sub->addAction(QObject::tr("Whole disk"));
            actWhole->setCheckable(true);
            actWhole->setChecked(!showPartitions);
            grp->addAction(actWhole);

            auto *actParts = sub->addAction(QObject::tr("Individual partitions (%1)").arg(diskInfo.partList.size()));
            actParts->setCheckable(true);
            actParts->setEnabled(!diskInfo.partList.empty());
            actParts->setChecked(showPartitions);
            grp->addAction(actParts);

            QAction *sel = menu.exec(target->mapToGlobal(pos));
            if (sel == actWhole || sel == actParts) {
                showPartitions = (sel == actParts);
                chartStack->setCurrentIndex(showPartitions ? 1 : 0);
                QString key = "showPartitions_" + QString::fromStdString(diskInfo.device);
                QSettings("FutureTechnology", "FutureTaskManager").setValue(key, showPartitions);
            }
        });
    }

    QLabel* makeBig(QGridLayout *g, const QString &cap, const QString &val, int r, int c) {
        auto *box = new QVBoxLayout();
        box->setSpacing(1);
        auto *lc = new QLabel(cap); lc->setStyleSheet("font-size: 11px; color: gray;");
        auto *lv = new QLabel(val); lv->setStyleSheet("font-size: 18px; font-weight: 500;");
        box->addWidget(lc); box->addWidget(lv);
        g->addLayout(box, r, c);
        return lv;
    }

    void addSpec(QGridLayout *g, int row, const QString &k, const QString &v) {
        auto *lk = new QLabel(k); lk->setStyleSheet("font-size: 12px; color: gray;");
        auto *lv = new QLabel(v); lv->setStyleSheet("font-size: 12px; font-weight: 500;");
        g->addWidget(lk, row, 0); g->addWidget(lv, row, 1);
    }
};
