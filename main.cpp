/*
##############################################################################
# Universidade de de Brasilia                                                #
# Instituto de Ciencia Exatas                                                #
# Programa de Pos Graduacao em Informatica                                   #
#                                                                            #
# Projeto e Complexidade de Algoritmos - 2026/1                           	 #
#                                                                            #
# Alunos: Joao Francisco Gomes Targino                                       #
#         Kássio Medeiros Alves                                              #
# Matriculas: 252107289                                                      #
#             261112591                                                      #
# Versao do Compilador: GCC 16.1.0                                           #
#                                                                            #
##############################################################################
*/
#include "auxiliares.h"
#include "protocolos.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

/*
BWA representa conjuntos como vetores de bits e faz AND bit a bit.
BWA e eficiente quando o universo e pequeno, mas cresce com 2^sigma.
PWC compara todos os pares possiveis e por isso tem pior caso O(n^2).
SCS_SORT recebe os conjuntos ordenados, compara apenas vizinhos e evita a comparacao todos-contra-todos.
A etapa final do SCS_SORT ordena candidatos para esconder a posicao dos matches.
Como a ordenacao final precisa ter sequencia fixa de comparacoes, usamos Bitonic Sorting Network.
*/
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

