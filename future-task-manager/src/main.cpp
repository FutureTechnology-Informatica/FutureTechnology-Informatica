#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QTimer>
#include <QElapsedTimer>
#include <QToolButton>
#include <QDialog>
#include <QRadioButton>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QSettings>
#include <QTranslator>
#include <QLocale>
#include <QIcon>
#include <QVector>
#include "sparkline.h"
#include "cpuwidget.h"
#include "memwidget.h"
#include "diskwidget.h"
#include "netwidget.h"

struct SidebarEntry {
    QString id;
    SparklineWidget *spark;
    QLabel *title;
    QLabel *subtitle;
};

class FutureTaskManager : public QMainWindow {
public:
    FutureTaskManager(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Future Task Manager");
        setWindowIcon(QIcon::fromTheme("future-task-manager"));
        resize(1080, 720);

        QSettings settings("FutureTechnology", "FutureTaskManager");
        intervalMs = settings.value("updateIntervalMs", 1000).toInt();
        hideVirtualDisks = settings.value("hideVirtualDisks", true).toBool();

        auto *central = new QWidget(this);
        auto *mainLayout = new QHBoxLayout(central);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        auto *leftPanel = new QWidget(this);
        leftPanel->setFixedWidth(260);
        leftPanel->setObjectName("leftPanel");
        leftPanel->setStyleSheet("QWidget#leftPanel { border-right: 1px solid palette(mid); }");

        auto *leftLayout = new QVBoxLayout(leftPanel);
        leftLayout->setContentsMargins(0, 0, 0, 0);
        leftLayout->setSpacing(0);

        auto *topBar = new QHBoxLayout();
        topBar->setContentsMargins(14, 10, 8, 8);
        auto *lblBrand = new QLabel(QObject::tr("Performance"));
        lblBrand->setStyleSheet("font-weight: bold; font-size: 13px; color: gray;");

        auto *btnGear = new QToolButton(this);
        btnGear->setText("⚙");
        btnGear->setToolTip(QObject::tr("Settings"));
        btnGear->setStyleSheet(
            "QToolButton { border: none; font-size: 16px; padding: 4px 6px; border-radius: 4px; }"
            "QToolButton:hover { background-color: palette(midlight); }"
        );
        connect(btnGear, &QToolButton::clicked, this, &FutureTaskManager::openSettings);

        topBar->addWidget(lblBrand);
        topBar->addStretch();
        topBar->addWidget(btnGear);

        sidebar = new QListWidget(this);
        sidebar->setStyleSheet(
            "QListWidget { border: none; background: transparent; outline: none; }"
            "QListWidget::item { padding: 4px 6px; border-radius: 4px; }"
            "QListWidget::item:selected { background-color: rgba(61, 174, 233, 45); }"
        );

        stack = new QStackedWidget(this);
        connect(sidebar, &QListWidget::currentRowChanged, stack, &QStackedWidget::setCurrentIndex);

        auto *lblFooter = new QLabel("Future Technology Informática");
        lblFooter->setAlignment(Qt::AlignCenter);
        lblFooter->setStyleSheet("font-size: 11px; color: gray; padding: 10px;");

        leftLayout->addLayout(topBar);
        leftLayout->addWidget(sidebar);
        leftLayout->addWidget(lblFooter);

        mainLayout->addWidget(leftPanel);
        mainLayout->addWidget(stack);
        setCentralWidget(central);

        buildDevices();

        elapsedTimer.start();
        refreshAll();

        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &FutureTaskManager::refreshAll);
        applyInterval();
    }

private:
    QListWidget *sidebar;
    QStackedWidget *stack;
    CpuWidget *cpuPage = nullptr;
    MemWidget *memPage = nullptr;
    QVector<DiskWidget*> diskPages;
    QVector<NetWidget*> netPages;
    SidebarEntry entryCpu, entryMem;
    QVector<SidebarEntry> diskEntries, netEntries;
    QVector<QString> rowIds;
    QString currentHardwareSig;
    QTimer *timer;
    QElapsedTimer elapsedTimer;
    int intervalMs = 1000;
    bool hideVirtualDisks = true;

    QString computeHardwareSignature(std::vector<DiskInfo> &disks, std::vector<NetInfo> &nets) {
        QString sig;
        for (const auto &d : disks) {
            if (hideVirtualDisks && (d.device.rfind("zram", 0) == 0 || d.device.rfind("sr", 0) == 0)) continue;
            sig += "D:" + QString::fromStdString(d.device) + "(" + QString::fromStdString(d.partitions) + ");";
        }
        for (const auto &n : nets) {
            sig += "N:" + QString::fromStdString(n.iface) + ":" + QString::fromStdString(n.state) + ";";
        }
        return sig;
    }

    void buildDevices() {
        QString prevSelectedId = "cpu";
        int curRow = sidebar->currentRow();
        if (curRow >= 0 && curRow < rowIds.size()) prevSelectedId = rowIds[curRow];

        sidebar->blockSignals(true);
        sidebar->clear();
        while (stack->count() > 0) {
            QWidget *w = stack->widget(0);
            stack->removeWidget(w);
            delete w;
        }
        diskPages.clear();
        netPages.clear();
        diskEntries.clear();
        netEntries.clear();
        rowIds.clear();

        cpuPage = new CpuWidget();
        memPage = new MemWidget();
        entryCpu = addSidebarRow("cpu", "CPU", "0% 0.00 GHz", QColor("#117dbb"));
        entryMem = addSidebarRow("mem", QObject::tr("Memory"), "0.0/0.0 GB (0%)", QColor("#8b12ae"));
        stack->addWidget(cpuPage);
        stack->addWidget(memPage);

        auto disks = SysInfoReader::listPhysicalDisks();
        auto nets = SysInfoReader::listNetInterfaces();
        currentHardwareSig = computeHardwareSignature(disks, nets);

        int diskIdx = 0;
        for (const auto &d : disks) {
            if (hideVirtualDisks && (d.device.rfind("zram", 0) == 0 || d.device.rfind("sr", 0) == 0)) continue;
            auto *dw = new DiskWidget(diskIdx, d);
            diskPages.append(dw);
            QString id = "disk_" + dw->deviceName();
            QString label = QObject::tr("Disk %1 (%2)").arg(diskIdx).arg(dw->deviceName());
            diskEntries.append(addSidebarRow(id, label, "0%", QColor("#2e8b57")));
            stack->addWidget(dw);
            diskIdx++;
        }

        for (const auto &n : nets) {
            auto *nw = new NetWidget(n);
            netPages.append(nw);
            QString id = "net_" + nw->ifaceName();
            QString label = QObject::tr("Network (%1)").arg(nw->ifaceName());
            QString st = nw->isConnected() ? QObject::tr("Connected") : QObject::tr("Disconnected");
            netEntries.append(addSidebarRow(id, id, label, st, QColor("#b7472a")));
            stack->addWidget(nw);
        }

        int targetRow = rowIds.indexOf(prevSelectedId);
        if (targetRow < 0) targetRow = 0;
        sidebar->setCurrentRow(targetRow);
        stack->setCurrentIndex(targetRow);
        sidebar->blockSignals(false);
    }
    SidebarEntry addSidebarRow(const QString &id, const QString &titleText, const QString &subText, const QColor &color) {
        rowIds.append(id);
        auto *item = new QListWidgetItem(sidebar);
        item->setSizeHint(QSize(240, 58));

        auto *rowWidget = new QWidget();
        auto *hbox = new QHBoxLayout(rowWidget);
        hbox->setContentsMargins(6, 4, 6, 4);
        hbox->setSpacing(10);

        auto *spark = new SparklineWidget(color);
        auto *vbox = new QVBoxLayout();
        vbox->setSpacing(2);
        vbox->setContentsMargins(0, 0, 0, 0);

        auto *lblT = new QLabel(titleText);
        lblT->setStyleSheet("font-size: 14px; font-weight: 500;");
        auto *lblS = new QLabel(subText);
        lblS->setStyleSheet("font-size: 11px; color: gray;");

        vbox->addStretch();
        vbox->addWidget(lblT);
        vbox->addWidget(lblS);
        vbox->addStretch();

        hbox->addWidget(spark);
        hbox->addLayout(vbox, 1);

        sidebar->setItemWidget(item, rowWidget);
        return {id, spark, lblT, lblS};
    }

    SidebarEntry addSidebarRow(const QString &id, const QString &, const QString &titleText, const QString &subText, const QColor &color) {
        return addSidebarRow(id, titleText, subText, color);
    }

    void refreshAll() {
        auto disks = SysInfoReader::listPhysicalDisks();
        auto nets = SysInfoReader::listNetInterfaces();
        if (computeHardwareSignature(disks, nets) != currentHardwareSig) {
            buildDevices();
        }

        double sec = elapsedTimer.restart() / 1000.0;
        if (sec <= 0.0) sec = 1.0;

        double cpuPct = 0.0;
        CpuDetails cpuD = cpuPage->updateData(cpuPct);
        MemMetrics mem = memPage->updateData();

        entryCpu.spark->pushValue(cpuPct);
        entryCpu.subtitle->setText(QString("%1% %2 GHz").arg(cpuPct, 0, 'f', 0).arg(cpuD.currentGhz, 0, 'f', 2));

        entryMem.spark->pushValue(mem.percent);
        entryMem.subtitle->setText(QString("%1/%2 GB (%3%)")
                                   .arg(mem.usedGb, 0, 'f', 1).arg(mem.totalGb, 0, 'f', 1).arg(mem.percent, 0, 'f', 0));

        for (int i = 0; i < diskPages.size(); ++i) {
            double dPct = diskPages[i]->updateData(sec);
            diskEntries[i].spark->pushValue(dPct);
            diskEntries[i].subtitle->setText(QString("%1%").arg(dPct, 0, 'f', 0));
        }

        for (int i = 0; i < netPages.size(); ++i) {
            auto spd = netPages[i]->updateData(sec);
            netEntries[i].spark->pushValue(netPages[i]->lastPercent());
            if (!netPages[i]->isConnected() && spd.first == "0 Kbps" && spd.second == "0 Kbps") {
                netEntries[i].subtitle->setText(QObject::tr("Disconnected"));
            } else {
                netEntries[i].subtitle->setText(QString("S: %1 R: %2").arg(spd.second, spd.first));
            }
        }
    }

    void openSettings() {
        QDialog dlg(this);
        dlg.setWindowTitle(QObject::tr("Settings") + " — Future Task Manager");
        dlg.setFixedWidth(400);

        auto *layout = new QVBoxLayout(&dlg);

        auto *groupSpeed = new QGroupBox(QObject::tr("Update speed"));
        auto *vbox = new QVBoxLayout(groupSpeed);

        auto *rbHigh   = new QRadioButton(QObject::tr("High (0.5 second)"));
        auto *rbNormal = new QRadioButton(QObject::tr("Normal (1 second)"));
        auto *rbLow    = new QRadioButton(QObject::tr("Low (2 seconds)"));
        auto *rbPaused = new QRadioButton(QObject::tr("Paused"));

        if (intervalMs == 500) rbHigh->setChecked(true);
        else if (intervalMs == 2000) rbLow->setChecked(true);
        else if (intervalMs == 0) rbPaused->setChecked(true);
        else rbNormal->setChecked(true);

        vbox->addWidget(rbHigh);
        vbox->addWidget(rbNormal);
        vbox->addWidget(rbLow);
        vbox->addWidget(rbPaused);
        layout->addWidget(groupSpeed);

        auto *groupDisks = new QGroupBox(QObject::tr("Disks"));
        auto *vboxDisks = new QVBoxLayout(groupDisks);
        auto *chkHideVirtual = new QCheckBox(QObject::tr("Hide virtual and optical drives (zram, sr/DVD)"));
        chkHideVirtual->setChecked(hideVirtualDisks);
        vboxDisks->addWidget(chkHideVirtual);
        layout->addWidget(groupDisks);

        auto *groupLang = new QGroupBox(QObject::tr("Language (requires restart)"));
        auto *vboxLang = new QVBoxLayout(groupLang);
        auto *cmbLang = new QComboBox();
        cmbLang->addItem(QObject::tr("System Automatic"), "auto");
        cmbLang->addItem("Português (Brasil)", "pt_BR");
        cmbLang->addItem("English", "en");
        cmbLang->addItem("Español", "es");

        QSettings stRead("FutureTechnology", "FutureTaskManager");
        QString curLang = stRead.value("language", "auto").toString();
        int idx = cmbLang->findData(curLang);
        if (idx >= 0) cmbLang->setCurrentIndex(idx);
        vboxLang->addWidget(cmbLang);
        layout->addWidget(groupLang);

        auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        layout->addWidget(btns);

        if (dlg.exec() == QDialog::Accepted) {
            if (rbHigh->isChecked()) intervalMs = 500;
            else if (rbNormal->isChecked()) intervalMs = 1000;
            else if (rbLow->isChecked()) intervalMs = 2000;
            else if (rbPaused->isChecked()) intervalMs = 0;

            bool newHide = chkHideVirtual->isChecked();
            QString newLang = cmbLang->currentData().toString();

            QSettings st("FutureTechnology", "FutureTaskManager");
            st.setValue("updateIntervalMs", intervalMs);
            st.setValue("hideVirtualDisks", newHide);
            st.setValue("language", newLang);

            if (newHide != hideVirtualDisks) {
                hideVirtualDisks = newHide;
                buildDevices();
                refreshAll();
            }
            applyInterval();
        }
    }

    void applyInterval() {
        if (intervalMs <= 0) timer->stop();
        else { elapsedTimer.restart(); timer->start(intervalMs); }
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName("FutureTechnology");
    app.setApplicationName("Future Task Manager");

    QSettings st("FutureTechnology", "FutureTaskManager");
    QString lang = st.value("language", "auto").toString();
    if (lang == "auto") lang = QLocale::system().name();

    QTranslator translator;
    if (lang.startsWith("pt")) {
        if (translator.load(":/i18n/future-task-manager_pt_BR.qm")) app.installTranslator(&translator);
    } else if (lang.startsWith("es")) {
        if (translator.load(":/i18n/future-task-manager_es.qm")) app.installTranslator(&translator);
    }

    FutureTaskManager win;
    win.show();
    return app.exec();
}
