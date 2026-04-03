def sum_array(arr: list) -> int:
    total: int = 0
    for x in arr:
        total = total + x
    return total

arr = [1, 2, 3, 4, 5]
result: int = sum_array(arr)
print("sum_array=", result)