# Future Technology Informática — Future Task Manager

🌐 [English](README.md) | **Português (Brasil)** | [Español](README.es.md)

Ferramentas leves de monitoramento de desempenho para Linux, idealizadas e criadas por **Paulo Roberto Jentsch** (**Future Technology Informática**) sob a licença **GPL-3.0**.

## Aplicativos
* **Future Task Manager (GUI v1.1.0):** Monitor gráfico em C++17 e Qt 6 com gráficos em tempo real (CPU, núcleos lógicos, Memória, Swap, Caches, Discos físicos, partições individuais e Rede), detecção USB Hot-Plug, informações da distro/kernel, temas claro/escuro e troca dinâmica de idioma.
* **Future Task Manager CLI (v1.0.0):** Monitor de terminal em C++17 e ncursesw com gráficos Braille Unicode suaves e filtros dedicados (`--only-cpu`, `--only-memory`, `--only-disk`, `--only-network`, `--interval`).

## Instalação Rápida (Debian / Ubuntu / Linux Mint / Raspberry Pi OS)
```bash
echo "deb [trusted=yes lang=none] https://futuretechnology-informatica.github.io/FutureTechnology-Informatica/apt-repo ./" | sudo tee /etc/apt/sources.list.d/future-technology.list
sudo apt update
sudo apt install future-task-manager future-task-manager-cli
```

## Suporte Multi-Arquitetura e Multi-Distro
Pacotes prontos (`.deb`, `.rpm`, `.txz`, `.tar.gz`) e código-fonte (`.dsc`) estão disponíveis neste repositório para **7 arquiteturas**: `amd64`, `i386`, `arm64`, `armhf`, `riscv64`, `ppc64el` e `s390x`.
