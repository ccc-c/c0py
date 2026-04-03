# Test with complex list comprehension and filter

numbers = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]

# List comprehension with condition
even_squares = [x**2 for x in numbers if x % 2 == 0]
print("even_squares:", even_squares)

# Filter and map
filtered = list(filter(lambda x: x > 5, numbers))
print("filtered:", filtered)

# Nested list
matrix = [[i*3+j for j in range(3)] for i in range(3)]
print("matrix:", matrix)