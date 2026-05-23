#pragma once

#include "tipos.h"

#include <string>
#include <vector>

int pow2(int sigma);
int nextPowerOfTwo(int x);
void printVector(const std::string& label, const std::vector<int>& v);
std::string measuredOperationsFor(const Stats& stats);
void printComparisonTable(const Stats& bwa, const Stats& pwc, const Stats& scs);
bool sameSet(std::vector<int> a, std::vector<int> b);
CsvInput readCsvInput(const std::string& path);
