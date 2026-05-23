#include "auxiliares.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;
/*Funcoes auxiliares do código*/

/*
Calcula 2^sigma, com verificação de overflow
*/
int pow2(int sigma) {
    if (sigma < 0 || sigma >= 31) {
        throw invalid_argument("sigma deve estar no intervalo [0, 30] para caber em int.");
    }
    return 1 << sigma;
}

/*
Calcula a próxima potência de 2 maior ou igual a x, com verificação de overflow
Usado para determinar o tamanho do vetor de bits e para padding em bitonic merge/sort
*/
int nextPowerOfTwo(int x) {
    if (x <= 1) {
        return 1;
    }

    int p = 1;
    while (p < x) {
        if (p > (1 << 30)) {
            throw overflow_error("Proxima potencia de 2 excede o limite de int.");
        }
        p <<= 1;
    }
    return p;
}

/*
Print dos vetores A, B e do resultado da interseccao
*/
void printVector(const string& label, const vector<int>& v) {
    cout << label << " = [";
    for (size_t i = 0; i < v.size(); ++i) {
        if (i > 0) {
            cout << ", ";
        }
        cout << v[i];
    }
    cout << "]\n";
}

/*
Quantidade de operações medidas para cada algoritmo
*/
string measuredOperationsFor(const Stats& stats) {
    if (stats.algorithmName == "BWA") {
        return to_string(stats.mainOperations) + " ANDs";
    }
    if (stats.algorithmName == "PWC") {
        return to_string(stats.mainOperations) + " comparacoes";
    }

    long long compareExchanges =
        stats.bitonicMergeCompareExchanges + stats.bitonicSortCompareExchanges;

    return to_string(compareExchanges) + " comparacoes; total=" +
           to_string(stats.mainOperations);
}

/*
Tabela comparativa final, mostrando o numero de operacoes medidas e o tempo de execucao de cada algoritmo
*/
void printComparisonTable(const Stats& bwa, const Stats& pwc, const Stats& scs) {
    cout << "\nQuadro comparativo final\n";
    cout << left
         << setw(12) << "Algoritmo"
         << setw(42) << "Operacoes medidas"
         << setw(13) << "Tempo\n";

    cout << string(200, '-') << "\n";

    auto printRow = [](const Stats& stats) {
        cout << left
             << setw(12) << stats.algorithmName
             << setw(42) << measuredOperationsFor(stats);

        cout << right << fixed << setprecision(4)
             << setw(9) << stats.executionTimeMs << " ms  \n";
    };

    printRow(bwa);
    printRow(pwc);
    printRow(scs);
}

/*
Verifica se dois vetores representam o mesmo conjunto
*/
bool sameSet(vector<int> a, vector<int> b) {
    sort(a.begin(), a.end());
    sort(b.begin(), b.end());
    return a == b;
}

/*
Leitura do CSV de entrada
*/
CsvInput readCsvInput(const string& path) {
    ifstream file(path);
    if (!file) {
        throw runtime_error("Nao foi possivel abrir o CSV: " + path);
    }

    CsvInput input;
    input.sigma = -1;

    string line;
    getline(file, line);

    while (getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        string sigmaText, setText, valueText;
        stringstream ss(line);

        getline(ss, sigmaText, ',');
        getline(ss, setText, ',');
        getline(ss, valueText, ',');

        int sigma = stoi(sigmaText);
        int value = stoi(valueText);

        if (input.sigma == -1) {
            input.sigma = sigma;
        }

        if (setText == "A") {
            input.A.push_back(value);
        } else if (setText == "B") {
            input.B.push_back(value);
        } else {
            throw runtime_error("Set invalido no CSV: " + setText);
        }
    }

    if (input.sigma == -1) {
        throw runtime_error("Sigma nao foi lido do CSV: " + path);
    }

    return input;
}
