#!/usr/bin/env bash
# Konfiguruje (raz), buduje i uruchamia projekt. Argumenty przekazywane do binarki.
#   ./run.sh              # zbuduj i odpal
#   ./run.sh --clean      # wyczyść build i zbuduj od zera
#   ./run.sh arg1 arg2    # argumenty trafiają do programu
set -euo pipefail

cd "$(dirname "$0")"
BUILD_DIR=build

if [[ "${1:-}" == "--clean" ]]; then
    rm -rf "$BUILD_DIR"
    shift
fi

cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j

exec "$BUILD_DIR/gomoku" "$@"
