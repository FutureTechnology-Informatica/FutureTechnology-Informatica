# Future Task Manager

**Future Task Manager** is a lightweight, native C++ and Qt 6 system performance monitor for Linux inspired by the Windows Task Manager, conceived and maintained by **Future Technology Informática**.

## Features
- **Real-Time Monitoring:** CPU (overall & logical processors grid), Memory (RAM, Swap, CPU L1/L2/L3 caches), Physical Disks (whole disk & individual partitions), and Network interfaces.
- **USB Hot-Plug Detection:** Automatically detects external storage drives and network adapters connected or removed in real time without restarting.
- **Multi-Language Support:** Built-in interface translations for English, Português (Brasil), and Español.
- **Multi-Architecture Ready:** Built and packaged for `amd64`, `i386`, `arm64`, `armhf`, `riscv64`, `ppc64el`, and `s390x`.
- **System Details:** Displays your current Linux distribution, kernel version, hardware models, and uptime.

## Building from Source (Debian / Ubuntu)
```bash
sudo apt install build-essential cmake ninja-build debhelper devscripts \
                 qt6-base-dev qt6-charts-dev qt6-l10n-tools qt6-tools-dev libxkbcommon-dev
dpkg-buildpackage -us -uc -b
```

## Author & Credits
- **Creator & Maintainer:** Paulo Roberto Jentsch — **Future Technology Informática**
- **Contact:** do_morro@hotmail.com
- **Acknowledgments:** Developed in collaboration with **Google Gemini** (AI assistant), whose extensive technical contribution helped architect, optimize, and package this project.

## License
Copyright (C) 2026 Paulo Roberto Jentsch / Future Technology Informática.  
Distributed under the **GNU General Public License v3.0 (GPL-3.0)**. See `LICENSE` for more details.
