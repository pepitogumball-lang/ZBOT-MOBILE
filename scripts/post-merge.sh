#!/bin/bash
# Post-merge setup para ZBOT-MOBILE.
#
# Este mod es C++ para Geode SDK en Geometry Dash. La compilacion
# real necesita el Geode SDK + Android NDK; los builds de produccion
# se hacen automaticamente en GitHub Actions (.github/workflows/build.yml).
#
# El script es idempotente: se puede ejecutar varias veces sin problemas.
set -e

echo "[post-merge] ZBOT-MOBILE: verificando entorno local..."

if command -v cmake &> /dev/null; then
    echo "[post-merge] cmake encontrado: $(cmake --version | head -1)"
else
    echo "[post-merge] AVISO: cmake no encontrado. Necesario para compilar localmente."
fi

echo "[post-merge] Para compilar: asegurate de tener GEODE_SDK definido y Android NDK instalado."
echo "[post-merge] Los builds automaticos se lanzan en GitHub Actions al hacer push a main."
echo "[post-merge] Listo."
exit 0
