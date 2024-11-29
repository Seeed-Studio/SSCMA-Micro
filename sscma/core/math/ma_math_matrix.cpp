#include "ma_math_matrix.h"
#include "ma_math_vectors.h"

#include <cmath>

namespace ma::math {

void softmax2D(float* data, size_t rows, size_t cols) {
    size_t size = rows * cols;
    for (size_t i = 0; i < size; i += cols) {
        softmax(&data[i], cols);
    }
}

void fastSoftmax2D(float* data, size_t rows, size_t cols) {
    size_t size = rows * cols;
    for (size_t i = 0; i < size; i += cols) {
        fastSoftmax(&data[i], cols);
    }
}

}