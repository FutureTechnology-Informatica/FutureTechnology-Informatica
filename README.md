# Future Technology Informática

Ferramentas leves de monitoramento e administração para Linux, criadas por **Paulo Roberto Jentsch** (**Future Technology Informática**) sob licença **GPL-3.0**.

## Resumo dos Aplicativos
* **Future Task Manager (GUI v1.1.0):** Monitor gráfico em C++17 e Qt 6 com gráficos em tempo real (CPU, RAM, Discos, Rede), detecção USB Hot-Plug, info do Kernel/Distribuição e multi-idioma/temas.
* **Future Task Manager CLI (v1.0.0):** Versão terminal em C++17 e ncursesw com gráficos em Braille e filtros específicos (`--only-cpu`, `--only-memory`, `--only-disk`, `--only-network`).

## Instalação Rápida (APT / RPM / Slackware)
Utilize as linhas do seu repositório local para configurar o APT:
```bash
echo "\${REPO_LINE}" | sudo tee /etc/apt/sources.list.d/future-technology.list
sudo apt update
sudo apt install future-task-manager future-task-manager-cli
```
*(Para a lista completa de arquiteturas suportadas [amd64, i386, arm64, armhf, riscv64, ppc64el, s390x], pacotes RPM, binários portáteis e detalhes de licenciamento GPL-3.0, consulte a documentação estruturada no repositório).*
