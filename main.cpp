#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_set>
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

int pow2(int sigma) {
    if (sigma < 0 || sigma >= 31) {
        throw invalid_argument("sigma deve estar no intervalo [0, 30] para caber em int.");
    }
    return 1 << sigma;
}

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

int automaticSigmaFromN(int n) {
    if (n <= 0) {
        throw invalid_argument("n deve ser positivo para calcular sigma automaticamente.");
    }

    // Como os elementos validos sao positivos, o universo contem 2^sigma - 1
    // valores uteis. Usamos 2n para permitir ate dois conjuntos disjuntos de
    // tamanho n, sem precisar ajustar sigma manualmente.
    long long requiredPositiveValues = 2LL * n;
    long long universeSize = 1;
    int sigma = 0;

    while (universeSize - 1 < requiredPositiveValues) {
        if (sigma >= 30) {
            throw overflow_error("n grande demais para calcular sigma dentro do limite de int.");
        }
        ++sigma;
        universeSize <<= 1;
    }

    return sigma;
}

vector<int> generateSet(int n, int universeSize, mt19937& rng) {
    if (n < 0) {
        throw invalid_argument("n nao pode ser negativo.");
    }
    if (universeSize <= 1 || n > universeSize - 1) {
        throw invalid_argument("Universo insuficiente para gerar n inteiros positivos distintos.");
    }

    vector<int> values;
    values.reserve(universeSize - 1);
    for (int x = 1; x < universeSize; ++x) {
        values.push_back(x);
    }

    shuffle(values.begin(), values.end(), rng);
    vector<int> result(values.begin(), values.begin() + n);
    sort(result.begin(), result.end());
    return result;
}

vector<int> makeSecondSetWithOverlap(
    const vector<int>& A,
    int n,
    int universeSize,
    int overlapPercent,
    mt19937& rng
) {
    if (n != static_cast<int>(A.size())) {
        throw invalid_argument("A deve ter tamanho n.");
    }
    if (overlapPercent < 0 || overlapPercent > 100) {
        throw invalid_argument("overlapPercent deve estar entre 0 e 100.");
    }
    if (universeSize <= 1 || n > universeSize - 1) {
        throw invalid_argument("Universo insuficiente para gerar B.");
    }

    int overlapCount = (n * overlapPercent) / 100;
    vector<int> shuffledA = A;
    shuffle(shuffledA.begin(), shuffledA.end(), rng);

    vector<int> B;
    B.reserve(n);
    unordered_set<int> chosen;
    unordered_set<int> inA(A.begin(), A.end());

    for (int i = 0; i < overlapCount; ++i) {
        B.push_back(shuffledA[i]);
        chosen.insert(shuffledA[i]);
    }

    vector<int> outsideA;
    for (int x = 1; x < universeSize; ++x) {
        if (inA.find(x) == inA.end()) {
            outsideA.push_back(x);
        }
    }

    shuffle(outsideA.begin(), outsideA.end(), rng);
    for (int x : outsideA) {
        if (static_cast<int>(B.size()) == n) {
            break;
        }
        if (chosen.insert(x).second) {
            B.push_back(x);
        }
    }

    // Se o universo for pequeno demais para manter exatamente o percentual,
    // completamos B com valores positivos ainda nao escolhidos.
    if (static_cast<int>(B.size()) < n) {
        vector<int> remaining;
        for (int x = 1; x < universeSize; ++x) {
            if (chosen.find(x) == chosen.end()) {
                remaining.push_back(x);
            }
        }

        shuffle(remaining.begin(), remaining.end(), rng);
        for (int x : remaining) {
            if (static_cast<int>(B.size()) == n) {
                break;
            }
            B.push_back(x);
            chosen.insert(x);
        }
    }

    if (static_cast<int>(B.size()) != n) {
        throw runtime_error("Nao foi possivel gerar B com n elementos distintos.");
    }

    sort(B.begin(), B.end());
    return B;
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

    // BWA e simples e rapido quando o universo e pequeno: cada elemento vira
    // uma posicao em um vetor de bits. Quando sigma cresce, o vetor tem
    // tamanho 2^sigma e a tecnica fica inviavel em memoria e tempo.
    vector<int> bitA(stats.universeSize, 0);
    vector<int> bitB(stats.universeSize, 0);

    for (int x : A) {
        if (x <= 0 || x >= stats.universeSize) {
            throw out_of_range("Elemento de A fora do universo positivo permitido.");
        }
        bitA[x] = 1;
    }

    for (int y : B) {
        if (y <= 0 || y >= stats.universeSize) {
            throw out_of_range("Elemento de B fora do universo positivo permitido.");
        }
        bitB[y] = 1;
    }

    vector<int> result;
    for (int i = 0; i < stats.universeSize; ++i) {
        ++stats.mainOperations;
        if ((bitA[i] & bitB[i]) == 1) {
            result.push_back(i);
        }
    }

    // Esta implementacao nao e privada. No protocolo real discutido no artigo,
    // os ANDs sensiveis seriam representados e avaliados dentro de garbled
    // circuits, sem revelar diretamente os vetores de bits.
    auto end = chrono::high_resolution_clock::now();
    stats.executionTimeMs = chrono::duration<double, milli>(end - start).count();
    stats.intersectionResult = result;
    return stats;
}

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

    // PWC e intuitivo: compara pares A[i] e B[j]. O short-circuit abaixo
    // economiza comparacoes quando encontra um match e marca B[j] como usado,
    // mas o pior caso continua quadratico.
    vector<int> result;
    vector<bool> matchedB(B.size(), false);

    for (size_t i = 0; i < A.size(); ++i) {
        for (size_t j = 0; j < B.size(); ++j) {
            if (!matchedB[j]) {
                ++stats.mainOperations;

                // No artigo, a funcao Equal(A[i], B[j]) seria um circuito
                // garbled. Aqui usamos apenas igualdade de int para fins
                // didaticos, sem criptografia ou oblivious transfer.
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

    // Bitonic Merge trabalha naturalmente com tamanho potencia de 2. Como os
    // elementos validos sao positivos, DUMMY=-1 pode ser usado como padding e
    // removido depois do merge.
    while (static_cast<int>(merged.size()) < paddedSize) {
        merged.push_back(DUMMY);
    }

    bitonicMergeAscending(merged, 0, static_cast<int>(merged.size()), counter);

    merged.erase(remove(merged.begin(), merged.end(), DUMMY), merged.end());
    return merged;
}

int dupSelect2(int a, int b) {
    if (a == b) {
        return a;
    }
    return DUMMY;
}

int dupSelect3(int a, int b, int c) {
    if (a == b || b == c) {
        return b;
    }
    return DUMMY;
}

vector<int> filterCandidates(const vector<int>& merged, long long& filteringComparisons) {
    vector<int> candidates;

    if (merged.size() < 2) {
        return candidates;
    }

    candidates.reserve(merged.size() / 2);

    for (size_t i = 1; i + 1 < merged.size(); i += 2) {
        // DupSelect3 contem duas igualdades conceituais. Em um circuito, a
        // estrutura seria fixa; aqui contamos as duas comparacoes didaticas.
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

    // A ordenacao local inicial pode usar std::sort, pois no protocolo do
    // artigo ela ocorre localmente em cada parte antes da etapa sensivel.
    vector<int> sortedA = A;
    vector<int> sortedB = B;
    sort(sortedA.begin(), sortedA.end());
    sort(sortedB.begin(), sortedB.end());

    // Ordenar faz valores iguais ficarem adjacentes depois do merge; assim o
    // SCS_SORT compara apenas vizinhos em vez de todos os pares.
    //
    // Em computacao privada, estruturas com fluxo dependente dos dados, como
    // merge comum, quicksort ou mergesort comum, nao representam bem a
    // execucao privada. A sequencia de comparacoes de Bitonic Merge e fixa,
    // por isso ela se encaixa melhor na ideia de circuitos garbled.
    vector<int> merged = bitonicMergeSortedSets(sortedA, sortedB, stats.bitonicMergeCompareExchanges);

    long long filteringComparisons = 0;
    vector<int> candidates = filterCandidates(merged, filteringComparisons);

    // Revelar os candidatos diretamente vazaria as posicoes dos matches no
    // vetor ordenado. O artigo usa uma etapa de embaralhamento/ordenacao para
    // esconder essa posicao. Aqui simulamos a ideia ordenando os candidatos
    // com uma Bitonic Sorting Network, cuja sequencia de comparacoes tambem e
    // fixa e mais compativel com garbled circuits do que mergesort comum.
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
    ofstream outputFile("saida.txt");
    if (!outputFile) {
        cerr << "Erro: nao foi possivel criar o arquivo saida.txt\n";
        return 1;
    }

    streambuf* originalCoutBuffer = cout.rdbuf(outputFile.rdbuf());

    try {
        const int n = 500000; // Tamanho dos conjuntos A e B
        const int overlapPercent = 50; // Percentual de sobreposicao entre A e B (0 a 100)
        const int sigma = automaticSigmaFromN(n); // Quantidade de bits para representar o universo, calculada automaticamente a partir de n
        const int universeSize = pow2(sigma); // Tamanho do universo, calculado como 2^sigma

        mt19937 rng(42);

        // Funcoes auxiliares de geração de conjuntos
        vector<int> A = generateSet(n, universeSize, rng);
        vector<int> B = makeSecondSetWithOverlap(A, n, universeSize, overlapPercent, rng);

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
        cout << "sigma automatico pelo n = " << sigma << "\n";
        cout << "universeSize = 2^sigma = " << universeSize << "\n";
        cout << "overlapPercent = " << overlapPercent << "%\n\n";

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
