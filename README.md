# Future Technology Informática — Future Task Manager

🌐 **English** | [Português (Brasil)](README.pt-BR.md) | [Español](README.es.md)

Lightweight system performance monitoring tools for Linux, conceived and created by **Paulo Roberto Jentsch** (**Future Technology Informática**) under the **GPL-3.0** license.

## Applications
* **Future Task Manager (GUI v1.1.0):** C++17 & Qt 6 graphical system monitor featuring real-time graphs (CPU, logical cores, Memory, Swap, Caches, Physical Disks, individual partitions, Network), USB hot-plug detection, Linux distribution/kernel info, light/dark themes, and dynamic language switching.
* **Future Task Manager CLI (v1.0.0):** Pure C++17 & ncursesw terminal monitor with smooth Unicode Braille charts and single-metric flags (`--only-cpu`, `--only-memory`, `--only-disk`, `--only-network`, `--interval`).

## Quick Installation (Debian / Ubuntu / Linux Mint / Raspberry Pi OS)
```bash
echo "deb [trusted=yes allow-insecure=yes lang=none] https://futuretechnology-informatica.github.io/FutureTechnology-Informatica/apt-repo ./" | sudo tee /etc/apt/sources.list.d/future-technology.list
sudo apt update
sudo apt install future-task-manager future-task-manager-cli
```

## Multi-Architecture & Multi-Distro Support
Pre-built packages (`.deb`, `.rpm`, `.txz`, `.tar.gz`) and source packages (`.dsc`) are available in this repository for **7 architectures**: `amd64`, `i386`, `arm64`, `armhf`, `riscv64`, `ppc64el`, and `s390x`.
