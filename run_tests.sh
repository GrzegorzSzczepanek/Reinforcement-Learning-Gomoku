#!/usr/bin/env bash
# Szybkie odpalenie testow.
# Logika jest w src/main.cpp razem z main(), wiec przy kompilacji testow
# podmieniamy jego main na game_main (-Dmain=game_main), zeby nie bylo konfliktu.

set -euo pipefail

# Katalog, w ktorym lezy ten skrypt (dziala niezaleznie skad go odpalasz).
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

OUT="$DIR/build/test_game"
mkdir -p "$DIR/build"

echo "==> Kompiluje testy..."
g++ -std=c++20 -Wall -Wextra -Dmain=game_main \
  -I"$DIR/src" \
  "$DIR/tests/test_game.cpp" \
  -o "$OUT"

echo "==> Uruchamiam testy..."
echo
"$OUT"
