#!/bin/bash
set -e

DSC_FILE="../future-task-manager_1.0.0-1.dsc"
DIST="trixie"
ARCHES=("amd64" "i386" "arm64" "armhf" "riscv64" "ppc64el" "s390x")

mkdir -p ~/.cache/sbuild
mkdir -p ../releases

for ARCH in "${ARCHES[@]}"; do
    echo "=========================================================="
    echo ">>> Preparando ambiente e compilando para: $ARCH"
    echo "=========================================================="

    TARBALL="$HOME/.cache/sbuild/${DIST}-${ARCH}.tar.zst"
    if [ ! -f "$TARBALL" ]; then
        mmdebstrap --arch="$ARCH" --variant=buildd "$DIST" "$TARBALL"
    fi

    sbuild --chroot-mode=unshare --arch="$ARCH" -d "$DIST" "$DSC_FILE"
    mv -f ../future-task-manager_*_${ARCH}.deb ../releases/ 2>/dev/null || true
done

echo "=========================================================="
echo ">>> Concluido! Pacotes gerados em ../releases/:"
ls -lh ../releases/
