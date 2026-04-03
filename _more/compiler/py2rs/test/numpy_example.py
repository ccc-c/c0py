# Test with third-party library import (should need LLM)

import numpy as np

def matrix_multiply(a: list, b: list) -> list:
    result = [[0, 0], [0, 0]]
    for i in range(2):
        for j in range(2):
            for k in range(2):
                result[i][j] += a[i][k] * b[k][j]
    return result

a = [[1, 2], [3, 4]]
b = [[5, 6], [7, 8]]
r = matrix_multiply(a, b)
print("matrix result:", r)