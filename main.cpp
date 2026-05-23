#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

const int DUMMY = -1;

struct Stats {
    string algorithmName;
    int n;
    int sigma;
    int universeSize;
    long long mainOperations;
    long long bitonicMergeCompareExchanges;
    long long bitonicSortCompareExchanges;
    double executionTimeMs;
    vector<int> intersectionResult;
};

struct CsvInput {
    int sigma;
    vector<int> A;
    vector<int> B;
};

// Calcula 2^sigma, com verificação de overflow
int pow2(int sigma) {
    if (sigma < 0 || sigma >= 31) {
        throw invalid_argument("sigma deve estar no intervalo [0, 30] para caber em int.");
    }
    return 1 << sigma;
}

// Calcula a próxima potência de 2 maior ou igual a x, com verificação de overflow
// Usado para determinar o tamanho do vetor de bits e para padding em bitonic merge/sort
// Pois o Bitonic Merge e a Bitonic Sorting Network exigem tamanhos que sejam potências de 2 
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

string measuredOperationsFor(const Stats& stats) {
    if (stats.algorithmName == "BWA") {
        return to_string(stats.mainOperations) + " ANDs";
    }
    if (stats.algorithmName == "PWC") {
        return to_string(stats.mainOperations) + " comparacoes";
    }

    long long compareExchanges =
        stats.bitonicMergeCompareExchanges + stats.bitonicSortCompareExchanges;

    return to_string(compareExchanges) + " compare-exchanges; total=" +
           to_string(stats.mainOperations);
}

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

bool sameSet(vector<int> a, vector<int> b) {
    sort(a.begin(), a.end());
    sort(b.begin(), b.end());
    return a == b;
}

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

// Algoritmo BWA: representa conjuntos como vetores de bits e faz AND bit a bit.
// Complexidade total vem de criar os vetores de bits (O(n)) e do AND bit a bit (O(2^sigma)).
// Simples e rapido quando o universo e pequeno.
// Quando sigma cresce, o vetor tem tamanho 2^sigma e a tecnica fica inviavel em memoria e tempo.
// Neste codigo sigma e calculado automaticamente a partir de n (o maior valor em A ou B)
Stats runBWA(const vector<int>& A, const vector<int>& B, int sigma) {
    if (A.size() != B.size()) {
        throw invalid_argument("BWA espera conjuntos A e B com o mesmo tamanho.");
    }

    Stats stats;
    stats.algorithmName = "BWA";
    stats.n = static_cast<int>(A.size());
    stats.sigma = sigma;
    stats.universeSize = pow2(sigma);
    stats.mainOperations = 0;
    stats.bitonicMergeCompareExchanges = 0;
    stats.bitonicSortCompareExchanges = 0;
    stats.executionTimeMs = 0.0;

    auto start = chrono::high_resolution_clock::now();

    vector<int> bitA(stats.universeSize, 0);
    vector<int> bitB(stats.universeSize, 0);

    for (int value : A) {
        bitA[value] = 1;
    }
    for (int value : B) {
        bitB[value] = 1;
    }

    vector<int> result;
    for (int i = 0; i < stats.universeSize; ++i) {
        ++stats.mainOperations;
        if ((bitA[i] & bitB[i]) == 1) {
            result.push_back(i);
        }
    }

    auto end = chrono::high_resolution_clock::now();
    stats.executionTimeMs = chrono::duration<double, milli>(end - start).count();
    stats.intersectionResult = result;
    return stats;
}

// Algoritmo PWC: compara todos os pares possiveis e por isso tem pior caso O(n^2).
// Melhor caso O(n) quando os conjuntos sao iguais, mas o short-circuit economiza comparacoes apenas nesse caso.
// Simples e intuitivo, mas ineficiente para n grande. 
Stats runPWC(const vector<int>& A, const vector<int>& B, int sigma) {
    if (A.size() != B.size()) {
        throw invalid_argument("PWC espera conjuntos A e B com o mesmo tamanho.");
    }

    Stats stats;
    stats.algorithmName = "PWC";
    stats.n = static_cast<int>(A.size());
    stats.sigma = sigma;
    stats.universeSize = pow2(sigma);
    stats.mainOperations = 0;
    stats.bitonicMergeCompareExchanges = 0;
    stats.bitonicSortCompareExchanges = 0;
    stats.executionTimeMs = 0.0;

    auto start = chrono::high_resolution_clock::now();

    vector<int> result;
    vector<bool> matchedB(B.size(), false);

    for (size_t i = 0; i < A.size(); ++i) {
        for (size_t j = 0; j < B.size(); ++j) {
            if (!matchedB[j]) {
                ++stats.mainOperations;

                if (A[i] == B[j]) {
                    result.push_back(A[i]);
                    matchedB[j] = true;
                    break;
                }
            }
        }
    }

    auto end = chrono::high_resolution_clock::now();
    stats.executionTimeMs = chrono::duration<double, milli>(end - start).count();
    stats.intersectionResult = result;
    return stats;
}

void compareExchange(vector<int>& v, int i, int j, bool ascending, long long& counter) {
    ++counter;

    if (ascending) {
        if (v[i] > v[j]) {
            swap(v[i], v[j]);
        }
    } else {
        if (v[i] < v[j]) {
            swap(v[i], v[j]);
        }
    }
}

void bitonicMergeAscending(vector<int>& v, int start, int size, long long& counter) {
    if (size <= 1) {
        return;
    }

    int half = size / 2;
    for (int i = start; i < start + half; ++i) {
        compareExchange(v, i, i + half, true, counter);
    }

    bitonicMergeAscending(v, start, half, counter);
    bitonicMergeAscending(v, start + half, half, counter);
}

// O Bitonic Merge tem complexidade O(n log n) e produz uma sequencia fixa de comparacoes
// Escolha foi feita para simular o que aconteceria no circuito garbled
// É um divisão e conquista que ordena uma sequencia bitonica (primeira metade crescente, segunda metade decrescente) em ordem crescente
vector<int> bitonicMergeSortedSets(vector<int> A, vector<int> B, long long& counter) {
    sort(A.begin(), A.end());
    sort(B.begin(), B.end());

    reverse(B.begin(), B.end());

    vector<int> merged;
    merged.reserve(A.size() + B.size());
    merged.insert(merged.end(), A.begin(), A.end());
    merged.insert(merged.end(), B.begin(), B.end());

    int originalSize = static_cast<int>(merged.size());
    int paddedSize = nextPowerOfTwo(originalSize);

    while (static_cast<int>(merged.size()) < paddedSize) {
        merged.push_back(DUMMY);
    }

    bitonicMergeAscending(merged, 0, static_cast<int>(merged.size()), counter);

    merged.erase(remove(merged.begin(), merged.end(), DUMMY), merged.end());
    return merged;
}

// A última comparação é feita entre os dois últimos elementos
int dupSelect2(int a, int b) {
    if (a == b) {
        return a;
    }
    return DUMMY;
}

// Como não há repetição ou a == b ou b == c, o que diminui o número de comparações necessárias 
int dupSelect3(int a, int b, int c) {
    if (a == b || b == c) {
        return b;
    }
    return DUMMY;
}

// Filtra candidatos comparando apenas vizinhos, já que não temos repetições nos conjuntos A e B
vector<int> filterCandidates(const vector<int>& merged, long long& filteringComparisons) {
    vector<int> candidates;

    if (merged.size() < 2) {
        return candidates;
    }

    candidates.reserve(merged.size() / 2);

    for (size_t i = 1; i + 1 < merged.size(); i += 2) {
        filteringComparisons += 2;
        candidates.push_back(dupSelect3(merged[i - 1], merged[i], merged[i + 1]));
    }

    filteringComparisons += 1;
    candidates.push_back(dupSelect2(merged[merged.size() - 2], merged[merged.size() - 1]));

    return candidates;
}

vector<int> bitonicSortNetwork(vector<int> v, long long& compareExchanges) {
    int n = static_cast<int>(v.size());
    int paddedSize = nextPowerOfTwo(n);
    while (static_cast<int>(v.size()) < paddedSize) {
        v.push_back(DUMMY);
    }
    n = static_cast<int>(v.size());

    for (int k = 2; k <= n; k <<= 1) {
        for (int j = k / 2; j > 0; j >>= 1) {
            for (int i = 0; i < n; ++i) {
                int partner = i ^ j;

                if (partner > i) {
                    bool ascending = ((i & k) == 0);
                    compareExchange(v, i, partner, ascending, compareExchanges);
                }
            }
        }
    }

    return v;
}

// SCS_SORT ordena os conjuntos, compara apenas vizinhos e evita a comparacao todos-contra-todos.
// Complexidade: O(n log^2 n). As ordenacoes locais e o Bitonic Merge custam O(n log n)
// Bitonic Sorting Network final domina com O(n log^2 n).
vector<int> scsSort(const vector<int>& A, const vector<int>& B, Stats& stats) {
    if (A.size() != B.size()) {
        throw invalid_argument("SCS_SORT espera conjuntos A e B com o mesmo tamanho.");
    }

    stats.algorithmName = "SCS_SORT";
    stats.n = static_cast<int>(A.size());
    stats.mainOperations = 0;
    stats.bitonicMergeCompareExchanges = 0;
    stats.bitonicSortCompareExchanges = 0;
    stats.executionTimeMs = 0.0;

    auto start = chrono::high_resolution_clock::now();

    // A ordenação nao entra no calculo de complexidade, pois a mesma seria feita localmente antes de entrar no protocolo
    vector<int> sortedA = A;
    vector<int> sortedB = B;
    sort(sortedA.begin(), sortedA.end());
    sort(sortedB.begin(), sortedB.end());

    vector<int> merged = bitonicMergeSortedSets(sortedA, sortedB, stats.bitonicMergeCompareExchanges);

    long long filteringComparisons = 0;
    vector<int> candidates = filterCandidates(merged, filteringComparisons);

    int paddedCandidateSize = nextPowerOfTwo(static_cast<int>(candidates.size()));
    while (static_cast<int>(candidates.size()) < paddedCandidateSize) {
        candidates.push_back(DUMMY);
    }

    vector<int> sortedCandidates = bitonicSortNetwork(candidates, stats.bitonicSortCompareExchanges);

    vector<int> result;
    for (int x : sortedCandidates) {
        if (x != DUMMY) {
            result.push_back(x);
        }
    }

    // Esta implementacao nao e privada: nao ha garbled circuits, oblivious
    // transfer ou criptografia. Ela apenas simula a estrutura algoritmica para
    // comparar custos e comportamento.
    stats.mainOperations =
        stats.bitonicMergeCompareExchanges +
        filteringComparisons +
        stats.bitonicSortCompareExchanges;

    auto end = chrono::high_resolution_clock::now();
    stats.executionTimeMs = chrono::duration<double, milli>(end - start).count();
    stats.intersectionResult = result;
    return result;
}

int main() {
    cout << "Escolha o CSV:\n";
    cout << "1. data/bwa_favoravel.csv\n";
    cout << "2. data/pwc_favoravel.csv\n";
    cout << "3. data/scs_sort_favoravel.csv\n";
    cout << "4. Informar caminho manualmente\n";
    cout << "Opcao: ";

    string csvPath;
    int csvOption = 1;
    cin >> csvOption;

    if (csvOption == 1) {
        csvPath = "data/bwa_favoravel.csv";
    } else if (csvOption == 2) {
        csvPath = "data/pwc_favoravel.csv";
    } else if (csvOption == 3) {
        csvPath = "data/scs_sort_favoravel.csv";
    } else {
        cout << "Caminho do CSV: ";
        cin >> csvPath;
    }

    ofstream outputFile("saida.txt");
    if (!outputFile) {
        cerr << "Erro: nao foi possivel criar o arquivo saida.txt\n";
        return 1;
    }

    streambuf* originalCoutBuffer = cout.rdbuf(outputFile.rdbuf());

    try {
        CsvInput input = readCsvInput(csvPath);
        int sigma = input.sigma;
        int universeSize = pow2(sigma);
        vector<int> A = input.A;
        vector<int> B = input.B;
        int n = static_cast<int>(A.size());

        // Execução dos algoritmos mais simples
        Stats bwa = runBWA(A, B, sigma);
        Stats pwc = runPWC(A, B, sigma);

        //Execução do SCS_SORT, mockando a estrutura de garbled circuits
        Stats scs;
        scs.sigma = sigma;
        scs.universeSize = universeSize;
        scsSort(A, B, scs);

        cout << "Parametros\n";
        cout << "n = " << n << "\n";
        cout << "CSV usado = " << csvPath << "\n";
        cout << "sigma lido do CSV = " << sigma << "\n";
        cout << "universeSize = 2^sigma = " << universeSize << "\n";
        cout << "\n";

        printVector("Conjunto A", A);
        printVector("Conjunto B", B);
        cout << "\n";

        printVector("Resultado BWA", bwa.intersectionResult);
        printVector("Resultado PWC", pwc.intersectionResult);
        printVector("Resultado SCS_SORT", scs.intersectionResult);

        bool allEqual =
            sameSet(bwa.intersectionResult, pwc.intersectionResult) &&
            sameSet(bwa.intersectionResult, scs.intersectionResult);

        // Prova de que todos algoritmos sao ótimos
        cout << "\nOs tres resultados sao iguais? " << (allEqual ? "Sim" : "Nao") << "\n";

        printComparisonTable(bwa, pwc, scs);
    } catch (const exception& ex) {
        cout.rdbuf(originalCoutBuffer);
        cerr << "Erro: " << ex.what() << "\n";
        return 1;
    }

    cout.rdbuf(originalCoutBuffer);
    return 0;
}

/*
BWA representa conjuntos como vetores de bits e faz AND bit a bit.
BWA e eficiente quando o universo e pequeno, mas cresce com 2^sigma.
PWC compara todos os pares possiveis e por isso tem pior caso O(n^2).
SCS_SORT ordena os conjuntos, compara apenas vizinhos e evita a comparacao todos-contra-todos.
A etapa final do SCS_SORT ordena candidatos para esconder a posicao dos matches.
Como a ordenacao final precisa ter sequencia fixa de comparacoes, usamos Bitonic Sorting Network.
Esta implementacao nao implementa garbled circuits reais.
Ela serve para demonstrar a estrutura dos algoritmos e suas complexidades.
*/
