# Future Technology Informática — Future Task Manager

🌐 [English](README.md) | [Português (Brasil)](README.pt-BR.md) | **Español**

Herramientas ligeras de monitoreo de rendimiento para Linux, concebidas y creadas por **Paulo Roberto Jentsch** (**Future Technology Informática**) bajo licencia **GPL-3.0**.

## Aplicaciones
* **Future Task Manager (GUI v1.1.0):** C++17 & Qt 6 graphical system monitor featuring real-time graphs (CPU, logical cores, Memory, Swap, Caches, Physical Disks, individual partitions, Network), USB hot-plug detection, Linux distribution/kernel info, light/dark themes, and dynamic language switching.
* **Future Task Manager CLI (v1.0.0):** Pure C++17 & ncursesw terminal monitor with smooth Unicode Braille charts and single-metric flags (`--only-cpu`, `--only-memory`, `--only-disk`, `--only-network`, `--interval`).

## Instalación

### Debian / Ubuntu / Linux Mint / Raspberry Pi OS (APT)
```bash
echo "deb [trusted=yes lang=none] https://futuretechnology-informatica.github.io/FutureTechnology-Informatica/apt-repo ./" | sudo tee /etc/apt/sources.list.d/future-technology.list
sudo apt update
sudo apt install future-task-manager future-task-manager-cli
```

### Fedora / RHEL / Rocky Linux / openSUSE (DNF / YUM)
```bash
cat << EOF | sudo tee /etc/yum.repos.d/future-technology.repo
[future-technology]
name=Future Technology Informatica
baseurl=https://futuretechnology-informatica.github.io/FutureTechnology-Informatica/rpm-repo
enabled=1
gpgcheck=0
EOF
sudo dnf install future-task-manager future-task-manager-cli
```

### Slackware (`.txz`)
Descargue el paquete correspondiente a su arquitectura en `slack-repo/` (verificando `SHA256SUMS`) e instale con:
```bash
sudo installpkg future-task-manager-1.1.0-<arch>-1ft.txz
sudo installpkg future-task-manager-cli-1.0.0-<arch>-1ft.txz
```

## Compilar desde el Código Fuente (Arch, Gentoo, Void o Compilaciones Personalizadas)
**Dependencias:** Compilador C++17 (`g++` o `clang++`), `cmake`, `ninja`, `qt6-base` + `qt6-tools` (para la versión gráfica) y `ncurses` (para el CLI).

```bash
# 1. Clone the repository
git clone https://github.com/FutureTechnology-Informatica/FutureTechnology-Informatica.git
cd FutureTechnology-Informatica

# 2. Build & Install GUI Version
cmake -S future-task-manager -B build-gui -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-gui && sudo cmake --install build-gui

# 3. Build & Install CLI Version
cmake -S future-task-manager-cli -B build-cli -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-cli && sudo cmake --install build-cli
```

## Soporte Multi-Arquitectura y Licencia
Pre-built packages and source packages (`.dsc`) are available for **7 architectures**: `amd64`, `i386`, `arm64`, `armhf`, `riscv64`, `ppc64el`, and `s390x`. Licensed under **GNU GPL-3.0**.
