int** MakeSpiral(int n) {
  int** spiral = new int*[n];
  for (int i = 0; i < n; ++i) {
    spiral[i] = new int[n];
  }
  if (n % 2 == 1) {
    spiral[n / 2][n / 2] = n * n;
  }

  int numb = 1;
  for (int i = 0; i < n / 2; i++) {
    for (int j = i; j < n - i; j++) {
      spiral[i][j] = numb++;
    }
    for (int j = i + 1; j < n - i - 1; j++) {
      spiral[j][n - i - 1] = numb++;
    }
    for (int j = n - i - 1; j >= i; j--) {
      spiral[n - i - 1][j] = numb++;
    }
    for (int j = n - i - 2; j >= i + 1; j--) {
      spiral[j][i] = numb++;
    }
  }
  return spiral;
}
