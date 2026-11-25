# Future Technology Informática — Future Task Manager

🌐 [English](README.md) | [Português (Brasil)](README.pt-BR.md) | **Español**

Herramientas ligeras de monitoreo de rendimiento para Linux, concebidas y creadas por **Paulo Roberto Jentsch** (**Future Technology Informática**) bajo licencia **GPL-3.0**.

## Aplicaciones
* **Future Task Manager (GUI v1.1.0):** Monitor gráfico en C++17 y Qt 6 con gráficos en tiempo real (CPU, núcleos lógicos, Memoria, Swap, Cachés, Discos físicos, particiones individuales y Red), detección USB Hot-Plug, información de distribución/kernel, temas claro/oscuro y cambio dinámico de idioma.
* **Future Task Manager CLI (v1.0.0):** Monitor de terminal en C++17 y ncursesw con gráficos Braille Unicode suaves y filtros dedicados (`--only-cpu`, `--only-memory`, `--only-disk`, `--only-network`, `--interval`).

## Instalación Rápida (Debian / Ubuntu / Linux Mint / Raspberry Pi OS)
```bash
echo "deb [trusted=yes lang=none] https://futuretechnology-informatica.github.io/FutureTechnology-Informatica/apt-repo ./" | sudo tee /etc/apt/sources.list.d/future-technology.list
sudo apt update
sudo apt install future-task-manager future-task-manager-cli
```

## Soporte Multi-Arquitectura y Multi-Distro
Paquetes compilados (`.deb`, `.rpm`, `.txz`, `.tar.gz`) y código fuente (`.dsc`) disponibles en este repositorio para **7 arquitecturas**: `amd64`, `i386`, `arm64`, `armhf`, `riscv64`, `ppc64el` y `s390x`.
