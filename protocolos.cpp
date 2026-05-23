#include "protocolos.h"

#include "auxiliares.h"

#include <algorithm>
#include <chrono>
#include <stdexcept>

using namespace std;

/*
Algoritmo BWA: representa conjuntos como vetores de bits e faz AND bit a bit.
Complexidade total vem de criar os vetores de bits (O(n)) e do AND bit a bit (O(2^sigma)).
Simples e rapido quando o universo e pequeno.
Quando sigma cresce, o vetor tem tamanho 2^sigma e a tecnica fica inviavel em memoria e tempo.
*/
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

/*
Algoritmo PWC: compara todos os pares possiveis e por isso tem pior caso O(n^2).
Melhor caso O(n) quando os conjuntos sao iguais, mas o short-circuit economiza comparacoes apenas nesse caso.
Simples e intuitivo, mas ineficiente para n grande. 
*/
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

/*
Funcao de comparação e troca usada tanto no Bitonic Merge quanto no Bitonic Sort.
*/
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

/*
Função recursiva do Bitonic Merge
*/
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

/*
O Bitonic Merge tem complexidade O(n log n) e produz uma sequencia fixa de comparacoes
Escolha foi feita para simular o que aconteceria no circuito garbled
É uma divisão e conquista que ordena uma sequencia bitonica (primeira metade crescente, segunda metade decrescente) em ordem crescente
*/
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

/*
A última comparação feita entre os dois últimos elementos
*/
int dupSelect2(int a, int b) {
    if (a == b) {
        return a;
    }
    return DUMMY;
}

/*
Como não há repetição ou a == b ou b == c podemos comparar os 3 ao mesmo tempo, o que diminui o número de comparações necessárias 
*/
int dupSelect3(int a, int b, int c) {
    if (a == b || b == c) {
        return b;
    }
    return DUMMY;
}

/*
Filtra candidatos comparando apenas vizinhos, já que não temos repetições nos conjuntos A e B
*/
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

/*
Implementação da rede de ordenação bitônica
Complexidade O(n log^2 n) devido ao número de comparações feitas
*/
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

    stats.mainOperations =
        stats.bitonicMergeCompareExchanges +
        filteringComparisons +
        stats.bitonicSortCompareExchanges;

    auto end = chrono::high_resolution_clock::now();
    stats.executionTimeMs = chrono::duration<double, milli>(end - start).count();
    stats.intersectionResult = result;
    return result;
}
