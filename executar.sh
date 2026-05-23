#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

CXX="${CXX:-g++}"

echo "Compilando projeto..."
"$CXX" -std=c++17 main.cpp auxiliares.cpp protocolos.cpp -O2 -o main

echo
echo "Executando programa..."
./main

echo
echo "Saida salva em saida.txt."
