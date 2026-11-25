#define _XOPEN_SOURCE_EXTENDED 1
#include <ncurses.h>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include "sysinfo.h"
#include "braille.h"

enum class FilterMode { All, OnlyCpu, OnlyMemory, OnlyDisk, OnlyNetwork };

struct TabItem {
    enum Type { Cpu, Memory, Disk, Network } type;
    std::string id, title, subtitle;
    int colorPair = 1;
    std::vector<double> history;
    std::vector<std::vector<double>> subHistory;
    DiskInfo diskInfo;
    NetInfo netInfo;
    DiskMetrics prevDisk;
    std::vector<DiskMetrics> prevParts;
    std::vector<double> partPeaks;
    NetMetrics prevNet;
    double netPeakBits = 100000.0;
};

static void pushHist(std::vector<double> &v, double val) {
    v.push_back(val);
    if (v.size() > 300) v.erase(v.begin());
}

static std::string fmtStr(const char *fmt, double a, double b = 0.0, double c = 0.0) {
    std::string buf(128, '\0');
    int n = std::snprintf(&buf.front(), buf.size(), fmt, a, b, c);
    if (n > 0) buf.resize(n);
    return buf;
}

static std::string fmtNet(double bps) {
    if (bps >= 1e9) return fmtStr("%.1f Gbps", bps / 1e9);
    if (bps >= 1e6) return fmtStr("%.1f Mbps", bps / 1e6);
    return fmtStr("%.0f Kbps", bps / 1e3);
}

static void printHelp() {
    std::printf(
        "Future Task Manager CLI v1.0.0 - Future Technology Informatica\n"
        "Conceived by Paulo Roberto Jentsch (GPL-3.0)\n\n"
        "Usage: future-task-manager-cli [OPTIONS]\n\n"
        "Options:\n"
        "  -c, --only-cpu         Show only CPU and logical processors monitor\n"
        "  -m, --only-memory      Show only Memory, Swap and CPU caches monitor\n"
        "  -d, --only-disk        Show only Physical Disks and partitions monitor\n"
        "  -n, --only-network     Show only Network interfaces monitor\n"
        "  -i, --interval <ms>    Set refresh interval in milliseconds (default: 1000)\n"
        "  -v, --version          Show version and license information\n"
        "  -h, --help             Show this help message and exit\n\n"
        "Keys: [Up/Down/Tab] Switch  [L] Logical CPUs  [P] Partitions  [V] Virtual Disks  [Q] Quit\n"
    );
}
int main(int argc, char *argv[]) {
    FilterMode mode = FilterMode::All;
    int intervalMs = 1000;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") { printHelp(); return 0; }
        if (arg == "-v" || arg == "--version") {
            std::printf("future-task-manager-cli 1.0.0 (GPL-3.0) - Future Technology Informatica\n");
            return 0;
        }
        if (arg == "-c" || arg == "--only-cpu") mode = FilterMode::OnlyCpu;
        else if (arg == "-m" || arg == "--only-memory") mode = FilterMode::OnlyMemory;
        else if (arg == "-d" || arg == "--only-disk") mode = FilterMode::OnlyDisk;
        else if (arg == "-n" || arg == "--only-network") mode = FilterMode::OnlyNetwork;
        else if ((arg == "-i" || arg == "--interval") && i + 1 < argc) {
            intervalMs = std::max(200, std::atoi(argv[++i]));
        } else {
            std::fprintf(stderr, "Unknown option: %s (use --help)\n", argv[i]);
            return 1;
        }
    }

    std::setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    timeout(100);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_CYAN, -1);
        init_pair(2, COLOR_MAGENTA, -1);
        init_pair(3, COLOR_GREEN, -1);
        init_pair(4, COLOR_RED, -1);
        init_pair(5, COLOR_BLACK, COLOR_CYAN);
        init_pair(6, COLOR_WHITE, -1);
    }

    bool showLogical = false, showPartitions = false, hideVirtual = true;
    int selected = 0;
    std::string hwSig;
    std::vector<TabItem> tabs;

    auto prevCpu = SysInfoReader::readCpu();
    CpuDetails cpuDet = SysInfoReader::readCpuDetails();
    MemMetrics memMet = SysInfoReader::readMemory();
    std::string osStr = SysInfoReader::getOsAndKernel();

    auto rebuildTabs = [&](const std::vector<DiskInfo> &disks, const std::vector<NetInfo> &nets) {
        std::string prevId = (selected >= 0 && selected < static_cast<int>(tabs.size())) ? tabs[selected].id : "";
        tabs.clear();

        if (mode == FilterMode::All || mode == FilterMode::OnlyCpu) {
            TabItem t; t.type = TabItem::Cpu; t.id = "cpu"; t.title = "CPU";
            t.subtitle = "0% 0.00 GHz"; t.colorPair = 1;
            t.subHistory.resize(std::max(1, static_cast<int>(prevCpu.size()) - 1));
            tabs.push_back(std::move(t));
        }
        if (mode == FilterMode::All || mode == FilterMode::OnlyMemory) {
            TabItem t; t.type = TabItem::Memory; t.id = "mem"; t.title = "Memory";
            t.subtitle = "0.0/0.0 GB"; t.colorPair = 2;
            tabs.push_back(std::move(t));
        }
        if (mode == FilterMode::All || mode == FilterMode::OnlyDisk) {
            int idx = 0;
            for (const auto &d : disks) {
                if (hideVirtual && (d.device.rfind("zram", 0) == 0 || d.device.rfind("sr", 0) == 0)) continue;
                TabItem t; t.type = TabItem::Disk; t.id = "disk_" + d.device;
                t.title = "Disk " + std::to_string(idx++) + " (" + d.device + ")";
                t.subtitle = "0%"; t.colorPair = 3; t.diskInfo = d;
                t.prevDisk = SysInfoReader::readDiskStats(d.device);
                t.subHistory.resize(d.partList.size());
                t.partPeaks.assign(d.partList.size(), 1.0);
                for (const auto &pt : d.partList) t.prevParts.push_back(SysInfoReader::readDiskStats(pt.name));
                tabs.push_back(std::move(t));
            }
        }
        if (mode == FilterMode::All || mode == FilterMode::OnlyNetwork) {
            for (const auto &n : nets) {
                TabItem t; t.type = TabItem::Network; t.id = "net_" + n.iface;
                t.title = "Net (" + n.iface + ")";
                t.subtitle = n.state; t.colorPair = 4; t.netInfo = n;
                t.prevNet = SysInfoReader::readNetStats(n.iface);
                tabs.push_back(std::move(t));
            }
        }
        selected = 0;
        for (int i = 0; i < static_cast<int>(tabs.size()); ++i) {
            if (tabs[i].id == prevId) { selected = i; break; }
        }
    };
    auto lastTick = std::chrono::steady_clock::now() - std::chrono::milliseconds(intervalMs);
    bool running = true;

    while (running) {
        auto now = std::chrono::steady_clock::now();
        double elapsedMs = std::chrono::duration<double, std::milli>(now - lastTick).count();

        if (elapsedMs >= intervalMs) {
            double sec = std::max(0.1, elapsedMs / 1000.0);
            lastTick = now;

            auto disks = SysInfoReader::listPhysicalDisks();
            auto nets = SysInfoReader::listNetInterfaces();
            std::string sig;
            for (const auto &d : disks) {
                if (hideVirtual && (d.device.rfind("zram", 0) == 0 || d.device.rfind("sr", 0) == 0)) continue;
                sig += "D:" + d.device + "(" + d.partitions + ");";
            }
            for (const auto &n : nets) sig += "N:" + n.iface + ":" + n.state + ";";
            if (sig != hwSig || tabs.empty()) { hwSig = sig; rebuildTabs(disks, nets); }

            auto currCpu = SysInfoReader::readCpu();
            cpuDet = SysInfoReader::readCpuDetails();
            memMet = SysInfoReader::readMemory();

            for (auto &t : tabs) {
                if (t.type == TabItem::Cpu) {
                    double pct = 0.0;
                    if (!currCpu.empty() && currCpu.size() == prevCpu.size()) {
                        long long totD = currCpu.front().total - prevCpu.front().total;
                        long long idlD = currCpu.front().idle - prevCpu.front().idle;
                        pct = totD > 0 ? (100.0 * (totD - idlD) / totD) : 0.0;
                        for (size_t c = 1; c < currCpu.size() && (c - 1) < t.subHistory.size(); ++c) {
                            long long ct = currCpu[c].total - prevCpu[c].total;
                            long long ci = currCpu[c].idle - prevCpu[c].idle;
                            pushHist(t.subHistory[c - 1], ct > 0 ? (100.0 * (ct - ci) / ct) : 0.0);
                        }
                    }
                    pushHist(t.history, pct);
                    t.subtitle = fmtStr("%.0f%% %.2f GHz", pct, cpuDet.currentGhz);
                } else if (t.type == TabItem::Memory) {
                    pushHist(t.history, memMet.percent);
                    t.subtitle = fmtStr("%.1f/%.1f GB (%.0f%%)", memMet.usedGb, memMet.totalGb, memMet.percent);
                } else if (t.type == TabItem::Disk) {
                    DiskMetrics curD = SysInfoReader::readDiskStats(t.diskInfo.device);
                    double act = std::clamp((std::max(0LL, curD.ioTicks - t.prevDisk.ioTicks) / (sec * 1000.0)) * 100.0, 0.0, 100.0);
                    pushHist(t.history, act);
                    t.subtitle = fmtStr("%.0f%%", act);
                    for (size_t p = 0; p < t.diskInfo.partList.size() && p < t.subHistory.size(); ++p) {
                        DiskMetrics pc = SysInfoReader::readDiskStats(t.diskInfo.partList[p].name);
                        double tot = (std::max(0LL, pc.readBytes - t.prevParts[p].readBytes) + std::max(0LL, pc.writeBytes - t.prevParts[p].writeBytes)) / (1048576.0 * sec);
                        if (tot > t.partPeaks[p]) t.partPeaks[p] = tot;
                        pushHist(t.subHistory[p], t.partPeaks[p] > 0 ? std::clamp((tot / t.partPeaks[p]) * 100.0, 0.0, 100.0) : 0.0);
                        t.prevParts[p] = pc;
                    }
                    t.prevDisk = curD;
                } else if (t.type == TabItem::Network) {
                    NetMetrics curN = SysInfoReader::readNetStats(t.netInfo.iface);
                    double rxB = (std::max(0LL, curN.rxBytes - t.prevNet.rxBytes) * 8.0) / sec;
                    double txB = (std::max(0LL, curN.txBytes - t.prevNet.txBytes) * 8.0) / sec;
                    double tot = rxB + txB;
                    if (tot > t.netPeakBits) t.netPeakBits = tot;
                    pushHist(t.history, t.netPeakBits > 0 ? (tot / t.netPeakBits) * 100.0 : 0.0);
                    t.subtitle = "S:" + fmtNet(txB) + " R:" + fmtNet(rxB);
                    t.prevNet = curN;
                }
            }
            prevCpu = currCpu;
        }

        erase();
        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        attron(A_BOLD); mvaddnstr(0, 1, "Future Task Manager CLI", cols - 2); attroff(A_BOLD);
        if (cols > static_cast<int>(osStr.size()) + 28) {
            attron(COLOR_PAIR(1)); mvaddstr(0, cols - static_cast<int>(osStr.size()) - 1, osStr.c_str()); attroff(COLOR_PAIR(1));
        }

        bool showSidebar = (mode == FilterMode::All && tabs.size() > 1 && cols >= 68);
        int mainX = showSidebar ? 27 : 1, mainW = cols - mainX - 1, mainH = rows - 6;

        if (showSidebar) {
            int sy = 2;
            for (int i = 0; i < static_cast<int>(tabs.size()) && sy + 2 < rows - 2; ++i) {
                if (i == selected) attron(COLOR_PAIR(5) | A_BOLD);
                mvaddnstr(sy, 1, (" " + tabs[i].title).c_str(), 24);
                mvaddnstr(sy + 1, 2, tabs[i].subtitle.c_str(), 23);
                if (i == selected) attroff(COLOR_PAIR(5) | A_BOLD);
                sy += 3;
            }
        }

        if (!tabs.empty() && mainW > 10 && mainH > 5) {
            const auto &cur = tabs[selected];
            if ((cur.type == TabItem::Cpu && showLogical || cur.type == TabItem::Disk && showPartitions) && !cur.subHistory.empty()) {
                int n = static_cast<int>(cur.subHistory.size());
                int gCols = std::max(1, static_cast<int>(std::ceil(std::sqrt(n))));
                int gRows = (n + gCols - 1) / gCols, cw = mainW / gCols, ch = mainH / gRows;
                for (int i = 0; i < n; ++i) {
                    std::string lbl = (cur.type == TabItem::Cpu)
                        ? ("CPU " + std::to_string(i) + ": " + fmtStr("%.0f%%", cur.subHistory[i].empty() ? 0.0 : cur.subHistory[i].back()))
                        : (cur.diskInfo.partList[i].name + (cur.diskInfo.partList[i].mountPoint.empty() ? "" : " (" + cur.diskInfo.partList[i].mountPoint + ")"));
                    BrailleChart::drawBoxChart(stdscr, 2 + (i / gCols) * ch, mainX + (i % gCols) * cw, ch, cw, cur.subHistory[i], cur.colorPair, lbl);
                }
            } else {
                std::string rInfo = (cur.type == TabItem::Cpu) ? cpuDet.model
                                  : (cur.type == TabItem::Memory) ? fmtStr("%.1f GB", memMet.totalGb)
                                  : (cur.type == TabItem::Disk) ? cur.diskInfo.model : cur.netInfo.adapterModel;
                BrailleChart::drawBoxChart(stdscr, 2, mainX, mainH, mainW, cur.history, cur.colorPair, cur.title + " - " + cur.subtitle, rInfo);
            }

            std::string info1, info2;
            if (cur.type == TabItem::Cpu) {
                info1 = fmtStr("Speed: %.2f / %.2f GHz", cpuDet.currentGhz, cpuDet.maxGhz) + " | Cores: " + std::to_string(cpuDet.physicalCores) + " | Logical: " + std::to_string(cpuDet.logicalProcessors) + " | Procs: " + std::to_string(cpuDet.processes);
                info2 = "Uptime: " + cpuDet.uptime + " | Virt: " + cpuDet.virtualization + " | L1: " + cpuDet.cacheL1 + " | L2: " + cpuDet.cacheL2 + " | L3: " + cpuDet.cacheL3;
            } else if (cur.type == TabItem::Memory) {
                info1 = fmtStr("In use: %.1f GB | Available: %.1f GB | Committed: %.1f GB | Cached: %.1f GB", memMet.usedGb, memMet.availGb, memMet.committedGb) + fmtStr(" (%.1f GB)", memMet.cachedGb);
                info2 = fmtStr("Swap: %.1f / %.1f GB", memMet.swapUsedGb, memMet.swapTotalGb) + " | L1: " + memMet.cacheL1 + " | L2: " + memMet.cacheL2 + " | L3: " + memMet.cacheL3;
            } else if (cur.type == TabItem::Disk) {
                info1 = "Device: /dev/" + cur.diskInfo.device + fmtStr(" | Capacity: %.1f GB", cur.diskInfo.sizeGb) + " | Activity: " + cur.subtitle;
                info2 = "Partitions: " + cur.diskInfo.partitions;
            } else if (cur.type == TabItem::Network) {
                info1 = "Interface: " + cur.netInfo.iface + " | Status: " + cur.netInfo.state + " | Traffic: " + cur.subtitle;
                info2 = "Adapter: " + cur.netInfo.adapterModel;
            }
            mvaddnstr(rows - 3, mainX, info1.c_str(), mainW);
            mvaddnstr(rows - 2, mainX, info2.c_str(), mainW);
        }

        attron(A_REVERSE);
        mvhline(rows - 1, 0, ' ', cols);
        std::string footer;
        if (tabs.size() > 1) footer += "[Up/Down/Tab] Switch  ";
        if (!tabs.empty()) {
            if (tabs[selected].type == TabItem::Cpu) footer += "[L] Logical CPUs  ";
            else if (tabs[selected].type == TabItem::Disk) footer += "[P] Partitions  [V] Virtual Disks  ";
        }
        footer += "[Q] Quit  |  Future Technology Informatica";
        mvaddnstr(rows - 1, 1, footer.c_str(), cols - 2);
        attroff(A_REVERSE);
        refresh();

        int ch = getch();
        if (ch == 'q' || ch == 'Q' || ch == 27) running = false;
        else if ((ch == KEY_DOWN || ch == '\t') && !tabs.empty()) selected = (selected + 1) % static_cast<int>(tabs.size());
        else if (ch == KEY_UP && !tabs.empty()) selected = (selected - 1 + static_cast<int>(tabs.size())) % static_cast<int>(tabs.size());
        else if (ch == 'l' || ch == 'L') showLogical = !showLogical;
        else if (ch == 'p' || ch == 'P') showPartitions = !showPartitions;
        else if (ch == 'v' || ch == 'V') { hideVirtual = !hideVirtual; hwSig.clear(); }
    }
    endwin();
    return 0;
}
