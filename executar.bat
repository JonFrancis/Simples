@echo off
setlocal

echo Compilando projeto...
g++ -std=c++17 main.cpp auxiliares.cpp protocolos.cpp -O2 -o main

if errorlevel 1 (
    echo.
    echo Erro na compilacao.
    exit /b 1
)

echo.
echo Executando programa...
main.exe

if errorlevel 1 (
    echo.
    echo Erro na execucao.
    exit /b 1
)

echo.
echo Saida salva em saida.txt.

endlocal
