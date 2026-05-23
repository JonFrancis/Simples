#pragma once

#include "tipos.h"

#include <vector>

Stats runBWA(const std::vector<int>& A, const std::vector<int>& B, int sigma);
Stats runPWC(const std::vector<int>& A, const std::vector<int>& B, int sigma);
std::vector<int> scsSort(const std::vector<int>& A, const std::vector<int>& B, Stats& stats);
