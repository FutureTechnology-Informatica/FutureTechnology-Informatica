#pragma once
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdio>
#include <sys/utsname.h>

namespace fs = std::filesystem;

struct CpuTimes { long long idle = 0, total = 0; };

struct CpuDetails {
    std::string model = "Unknown Processor";
    double currentGhz = 0.0, maxGhz = 0.0;
    int sockets = 1, physicalCores = 1, logicalProcessors = 1;
    std::string virtualization = "Disabled";
    std::string cacheL1 = "N/D", cacheL2 = "N/D", cacheL3 = "N/D";
    int processes = 0, threads = 0;
    std::string uptime = "0:00:00:00";
};

struct MemMetrics {
    double totalGb = 0, usedGb = 0, availGb = 0, cachedGb = 0, committedGb = 0, percent = 0;
    double swapTotalGb = 0, swapUsedGb = 0;
    std::string cacheL1 = "N/D", cacheL2 = "N/D", cacheL3 = "N/D";
};

struct PartitionInfo {
    std::string name, mountPoint;
    double sizeGb = 0.0;
};

struct DiskInfo {
    std::string device, model, partitions;
    double sizeGb = 0.0;
    std::vector<PartitionInfo> partList;
};

struct DiskMetrics { long long readBytes = 0, writeBytes = 0, ioTicks = 0; };

struct NetInfo { std::string iface, adapterModel, state; };

struct NetMetrics { long long rxBytes = 0, txBytes = 0; };

class SysInfoReader {
public:
    static std::string trim(const std::string &s) {
        auto b = s.find_first_not_of(" \t\r\n\"");
        if (b == std::string::npos) return "";
        auto e = s.find_last_not_of(" \t\r\n\"");
        return s.substr(b, e - b + 1);
    }

    static std::string getOsAndKernel() {
        std::string distro = "Linux";
        std::ifstream f("/etc/os-release");
        std::string line;
        while (std::getline(f, line)) {
            if (line.rfind("PRETTY_NAME=", 0) == 0) { distro = trim(line.substr(12)); break; }
        }
        struct utsname uts{};
        uname(&uts);
        return "SO: " + distro + "  |  Kernel: " + uts.release + " (" + uts.machine + ")";
    }

    static std::vector<CpuTimes> readCpu() {
        std::vector<CpuTimes> res;
        std::ifstream f("/proc/stat");
        std::string line;
        while (std::getline(f, line)) {
            if (line.rfind("cpu", 0) != 0) break;
            std::istringstream ss(line);
            std::string label;
            long long u = 0, n = 0, s = 0, id = 0, io = 0;
            ss >> label >> u >> n >> s >> id >> io;
            res.push_back({id + io, u + n + s + id + io});
        }
        return res;
    }

    static CpuDetails readCpuDetails() {
        CpuDetails d;
        std::ifstream f("/proc/cpuinfo");
        std::string line;
        double mhzSum = 0.0;
        int mhzCount = 0, curPhys = 0, curCore = 0;
        std::set<int> physIds;
        std::set<std::pair<int, int>> coreIds;

        while (std::getline(f, line)) {
            auto pos = line.find(':');
            if (pos == std::string::npos) continue;
            std::string key = trim(line.substr(0, pos));
            std::string val = trim(line.substr(pos + 1));

            if (key == "model name" || key == "Model" || key == "Hardware" || (key == "isa" && d.model == "Unknown Processor")) d.model = val;
            else if (key == "processor") { d.logicalProcessors = std::stoi(val) + 1; }
            else if (key == "cpu MHz") { mhzSum += std::stod(val); mhzCount++; }
            else if (key == "physical id") { curPhys = std::stoi(val); physIds.insert(curPhys); }
            else if (key == "core id") { curCore = std::stoi(val); coreIds.insert({curPhys, curCore}); }
            else if (key == "flags" && (val.find("vmx") != std::string::npos || val.find("svm") != std::string::npos))
                d.virtualization = "Enabled";
        }

        if (mhzCount > 0) {
            d.currentGhz = (mhzSum / mhzCount) / 1000.0;
            d.logicalProcessors = mhzCount;
        } else {
            std::ifstream fCur("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
            long long curKhz = 0;
            if (fCur >> curKhz) d.currentGhz = curKhz / 1000000.0;
        }
        d.sockets = physIds.empty() ? 1 : static_cast<int>(physIds.size());
        d.physicalCores = coreIds.empty() ? d.logicalProcessors : static_cast<int>(coreIds.size());

        std::ifstream fMax("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
        long long maxKhz = 0;
        d.maxGhz = (fMax >> maxKhz) ? (maxKhz / 1000000.0) : d.currentGhz;

        long long l1Kb = 0, l2Kb = 0, l3Kb = 0;
        std::string cacheBase = "/sys/devices/system/cpu/cpu0/cache";
        if (fs::exists(cacheBase)) {
            for (const auto &entry : fs::directory_iterator(cacheBase)) {
                int lvl = 0;
                std::string szStr;
                std::ifstream fl(entry.path() / "level");
                std::ifstream fsz(entry.path() / "size");
                if ((fl >> lvl) && (fsz >> szStr)) {
                    long long kb = std::stoll(szStr);
                    if (szStr.back() == 'M') kb *= 1024;
                    if (lvl == 1) l1Kb += kb;
                    else if (lvl == 2) l2Kb += kb;
                    else if (lvl == 3) l3Kb += kb;
                }
            }
        }
        auto fmtCache = [](long long kb) -> std::string {
            if (kb <= 0) return "N/D";
            if (kb >= 1024) {
                char buf[32];
                std::snprintf(buf, sizeof(buf), "%.1f MB", kb / 1024.0);
                return buf;
            }
            return std::to_string(kb) + " KB";
        };
        d.cacheL1 = fmtCache(l1Kb * d.physicalCores);
        d.cacheL2 = fmtCache(l2Kb * d.physicalCores);
        d.cacheL3 = fmtCache(l3Kb);

        std::ifstream fLoad("/proc/loadavg");
        double l1, l5, l15;
        std::string procStr;
        if (fLoad >> l1 >> l5 >> l15 >> procStr) {
            auto slash = procStr.find('/');
            if (slash != std::string::npos) d.threads = std::stoi(procStr.substr(slash + 1));
        }
        int procCount = 0;
        for (const auto &p : fs::directory_iterator("/proc")) {
            std::string name = p.path().filename().string();
            if (!name.empty() && std::isdigit(static_cast<unsigned char>(name.front()))) procCount++;
        }
        d.processes = procCount;

        std::ifstream fUp("/proc/uptime");
        double upSec = 0;
        if (fUp >> upSec) {
            long long s = static_cast<long long>(upSec);
            long long days = s / 86400; s %= 86400;
            long long hrs = s / 3600; s %= 3600;
            long long mins = s / 60; s %= 60;
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%lld:%02lld:%02lld:%02lld", days, hrs, mins, s);
            d.uptime = buf;
        }
        return d;
    }
    static MemMetrics readMemory() {
        MemMetrics m;
        std::ifstream f("/proc/meminfo");
        std::string key, unit;
        long long val = 0, memTotal = 0, memAvail = 0, buffers = 0, cached = 0, sReclaim = 0;
        long long swapTotal = 0, swapFree = 0, committed = 0;
        while (f >> key >> val >> unit) {
            if (key == "MemTotal:") memTotal = val;
            else if (key == "MemAvailable:") memAvail = val;
            else if (key == "Buffers:") buffers = val;
            else if (key == "Cached:") cached = val;
            else if (key == "SReclaimable:") sReclaim = val;
            else if (key == "SwapTotal:") swapTotal = val;
            else if (key == "SwapFree:") swapFree = val;
            else if (key == "Committed_AS:") committed = val;
        }
        if (memTotal > 0) {
            m.totalGb = memTotal / 1048576.0;
            m.availGb = memAvail / 1048576.0;
            m.usedGb = (memTotal - memAvail) / 1048576.0;
            m.cachedGb = (buffers + cached + sReclaim) / 1048576.0;
            m.committedGb = committed / 1048576.0;
            m.percent = 100.0 * (memTotal - memAvail) / memTotal;
        }
        m.swapTotalGb = swapTotal / 1048576.0;
        m.swapUsedGb = (swapTotal - swapFree) / 1048576.0;
        CpuDetails cd = readCpuDetails();
        m.cacheL1 = cd.cacheL1;
        m.cacheL2 = cd.cacheL2;
        m.cacheL3 = cd.cacheL3;
        return m;
    }

    static std::vector<DiskInfo> listPhysicalDisks() {
        std::vector<DiskInfo> disks;
        if (!fs::exists("/sys/block")) return disks;

        for (const auto &entry : fs::directory_iterator("/sys/block")) {
            std::string dev = entry.path().filename().string();
            if (dev.rfind("loop", 0) == 0 || dev.rfind("ram", 0) == 0 || dev.rfind("dm-", 0) == 0) continue;

            long long sectors = 0;
            std::ifstream fSz(entry.path() / "size");
            fSz >> sectors;
            if (sectors <= 0) continue;

            std::string model = "Disk Drive";
            std::ifstream fMod(entry.path() / "device/model");
            if (std::getline(fMod, model)) model = trim(model);

            std::string parts;
            std::vector<PartitionInfo> pList;
            for (const auto &sub : fs::directory_iterator(entry.path())) {
                std::string subName = sub.path().filename().string();
                if (subName.rfind(dev, 0) == 0 && subName != dev) {
                    if (!parts.empty()) parts += ", ";
                    parts += subName;
                    long long pSec = 0;
                    std::ifstream fpSz(sub.path() / "size");
                    fpSz >> pSec;
                    std::string mnt = "";
                    std::ifstream fMnt("/proc/mounts");
                    std::string mDev, mPt, mRest;
                    while (fMnt >> mDev >> mPt) {
                        std::getline(fMnt, mRest);
                        if (mDev == "/dev/" + subName) { mnt = mPt; break; }
                    }
                    pList.push_back({subName, mnt, (pSec * 512.0) / 1e9});
                }
            }
            std::sort(pList.begin(), pList.end(), [](const PartitionInfo &a, const PartitionInfo &b) {
                if (a.name.size() != b.name.size()) return a.name.size() < b.name.size();
                return a.name < b.name;
            });
            parts.clear();
            for (const auto &pi : pList) {
                if (!parts.empty()) parts += ", ";
                parts += pi.name;
            }
            if (parts.empty()) parts = "None";

            disks.push_back({dev, model, parts, (sectors * 512.0) / 1e9, pList});
        }
        std::sort(disks.begin(), disks.end(), [](const DiskInfo &a, const DiskInfo &b) {
            return a.device < b.device;
        });
        return disks;
    }

    static DiskMetrics readDiskStats(const std::string &targetDev) {
        std::ifstream f("/proc/diskstats");
        std::string line;
        while (std::getline(f, line)) {
            std::istringstream ss(line);
            int maj = 0, min = 0;
            std::string dev;
            if (!(ss >> maj >> min >> dev) || dev != targetDev) continue;
            std::vector<long long> v;
            long long num = 0;
            while (ss >> num) v.push_back(num);
            if (v.size() >= 10) return {v.at(2) * 512, v.at(6) * 512, v.at(9)};
            if (v.size() >= 4)  return {v.at(1) * 512, v.at(3) * 512, 0};
        }
        return {0, 0, 0};
    }

    static std::string lookupPciName(const std::string &pciId) {
        auto colon = pciId.find(':');
        if (colon == std::string::npos) return "";
        std::string ven = pciId.substr(0, colon);
        std::string dev = pciId.substr(colon + 1);
        for (auto &c : ven) c = std::tolower(c);
        for (auto &c : dev) c = std::tolower(c);

        std::ifstream f("/usr/share/misc/pci.ids");
        if (!f.is_open()) return "";
        std::string line, venName;
        bool inVendor = false;
        while (std::getline(f, line)) {
            if (line.empty() || line.front() == '#') continue;
            if (line.front() != '\t') {
                if (line.rfind(ven + "  ", 0) == 0) {
                    venName = trim(line.substr(6));
                    inVendor = true;
                } else if (inVendor) break;
            } else if (inVendor && line.size() > 7 && line.front() != '\t') {
                if (line.substr(1, 4) == dev) {
                    return venName + " " + trim(line.substr(7));
                }
            }
        }
        return venName;
    }

    static std::vector<NetInfo> listNetInterfaces() {
        std::vector<NetInfo> nets;
        if (!fs::exists("/sys/class/net")) return nets;

        for (const auto &entry : fs::directory_iterator("/sys/class/net")) {
            std::string iface = entry.path().filename().string();
            if (iface == "lo" || iface.rfind("veth", 0) == 0 || iface.rfind("docker", 0) == 0) continue;

            std::string state = "Disconnected";
            std::ifstream fOp(entry.path() / "operstate");
            std::string op;
            if ((fOp >> op) && op == "up") state = "Connected";

            std::string model = "Network Adapter";
            for (const auto &rel : {"device/uevent", "device/../uevent"}) {
                std::ifstream fUev(entry.path() / rel);
                std::string line;
                bool foundPci = false;
                while (std::getline(fUev, line)) {
                    if (line.rfind("PCI_ID=", 0) == 0) {
                        std::string resolved = lookupPciName(line.substr(7));
                        if (!resolved.empty()) { model = resolved; foundPci = true; }
                        break;
                    } else if (line.rfind("DRIVER=", 0) == 0 && model == "Network Adapter") {
                        model = "Driver: " + line.substr(7);
                    }
                }
                if (foundPci) break;
            }
            nets.push_back({iface, model, state});
        }
        return nets;
    }

    static NetMetrics readNetStats(const std::string &targetIface) {
        std::ifstream f("/proc/net/dev");
        std::string line;
        std::getline(f, line); std::getline(f, line);
        while (std::getline(f, line)) {
            auto colon = line.find(':');
            if (colon == std::string::npos) continue;
            std::string iface = trim(line.substr(0, colon));
            if (iface == targetIface) {
                std::istringstream ss(line.substr(colon + 1));
                long long rx, r1, r2, r3, r4, r5, r6, r7, tx;
                ss >> rx >> r1 >> r2 >> r3 >> r4 >> r5 >> r6 >> r7 >> tx;
                return {rx, tx};
            }
        }
        return {0, 0};
    }
};
