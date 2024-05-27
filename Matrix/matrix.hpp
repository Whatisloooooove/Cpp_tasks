#pragma once

#include <algorithm>
#include <vector>

template <size_t N, size_t M, typename T = int64_t>
class Matrix {
 private:
  std::vector<std::vector<T>> matrix_;

 public:
  Matrix() : matrix_(std::vector<std::vector<T>>(N, std::vector<T>(M))) {}

  Matrix(std::vector<std::vector<T>> matr) : matrix_(matr) {}

  Matrix(const T& elem) {
    for (size_t row = 0; row < N; ++row) {
      matrix_.push_back(std::vector<T>(M, elem));
    }
  }

  T& operator()(size_t row, size_t col) { return matrix_[row][col]; }

  T operator()(size_t row, size_t col) const { return matrix_[row][col]; }

  Matrix(const Matrix& other) : matrix_(other.matrix_) {}

  Matrix& operator=(const Matrix& other) { matrix_ = other.matrix_; }

  Matrix& operator+=(const Matrix& other) {
    for (size_t row = 0; row < N; ++row) {
      for (size_t col = 0; col < M; ++col) {
        matrix_[row][col] += other.matrix_[row][col];
      }
    }
    return *this;
  }

  Matrix& operator-=(const Matrix& other) {
    for (size_t row = 0; row < N; ++row) {
      for (size_t col = 0; col < M; ++col) {
        matrix_[row][col] -= other.matrix_[row][col];
      }
    }
    return *this;
  }

  Matrix& operator*=(const T& numb) {
    for (size_t row = 0; row < N; ++row) {
      for (size_t col = 0; col < M; ++col) {
        matrix_[row][col] *= numb;
      }
    }
    return *this;
  }

  Matrix<M, N, T> Transposed() const {
    Matrix<M, N, T> res;
    for (size_t col = 0; col < M; ++col) {
      for (size_t row = 0; row < N; ++row) {
        res(col, row) = matrix_[row][col];
      }
    }
    return res;
  }

  T Trace() const { return OutTrace(*this); }

  bool operator==(const Matrix& other) {
    for (size_t row = 0; row < N; ++row) {
      for (size_t col = 0; col < M; ++col) {
        if (matrix_[row][col] != other.matrix_[row][col]) {
          return false;
        }
      }
    }
    return true;
  }
};

template <size_t N, size_t M, typename T>
Matrix<N, M, T> operator+(const Matrix<N, M, T>& first,
                          const Matrix<N, M, T>& second) {
  Matrix copy = first;
  copy += second;
  return copy;
}

template <size_t N, size_t M, typename T>
Matrix<N, M, T> operator-(const Matrix<N, M, T>& first,
                          const Matrix<N, M, T>& second) {
  Matrix copy = first;
  copy -= second;
  return copy;
}

template <size_t N, size_t M, size_t L, typename T = int64_t>
Matrix<N, L, T> operator*(const Matrix<N, M, T>& first,
                          const Matrix<M, L, T>& second) {
  Matrix<N, L, T> copy;
  for (size_t row = 0; row < N; ++row) {
    for (size_t col_2 = 0; col_2 < L; ++col_2) {
      for (size_t col = 0; col < M; ++col) {
        copy(row, col_2) += first(row, col) * second(col, col_2);
      }
    }
  }
  return copy;
}

template <size_t N, size_t M, typename T = int64_t>
Matrix<N, M, T> operator*(const Matrix<N, M, T>& matr, const T& numb) {
  Matrix copy = matr;
  copy *= numb;
  return copy;
}

template <size_t N, typename T>
T OutTrace(const Matrix<N, N, T>& matr) {
  T res;
  for (size_t main = 0; main < N; ++main) {
    res += matr(main, main);
  }
  return res;
}
