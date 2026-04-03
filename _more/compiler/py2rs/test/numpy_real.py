# Test with actual numpy usage

import numpy as np

a = np.array([[1, 2], [3, 4]])
b = np.array([[5, 6], [7, 8]])

# Matrix multiplication using numpy
result = np.dot(a, b)

print("numpy dot result:", result)

# Element-wise operation
c = np.array([1, 2, 3, 4, 5])
squared = c ** 2
print("numpy squared:", squared)

# Sum
total = np.sum(c)
print("numpy sum:", total)