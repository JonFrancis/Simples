#pragma once

#include <string>
#include <vector>

inline const int DUMMY = -1;

struct Stats {
    std::string algorithmName;
    int n;
    int sigma;
    int universeSize;
    long long mainOperations;
    long long bitonicMergeCompareExchanges;
    long long bitonicSortCompareExchanges;
    double executionTimeMs;
    std::vector<int> intersectionResult;
};

struct CsvInput {
    int sigma;
    std::vector<int> A;
    std::vector<int> B;
};
