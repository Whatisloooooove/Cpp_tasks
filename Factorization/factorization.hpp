#pragma once
#include <cmath>
#include <vector>

std::vector<int> Factorize(int n) {
  std::vector<int> primes;

  for (int i = 2; i <= (int)sqrt(n); i++) {
    while (n % i == 0) {
      primes.push_back(i);
      n /= i;
    }
  }
  if (n != 1) {
    primes.push_back(n);
  }
  return primes;
}
