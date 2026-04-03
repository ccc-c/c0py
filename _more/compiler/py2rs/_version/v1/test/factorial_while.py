def factorial_while(n: int) -> int:
    result: int = 1
    while n > 1:
        result = result * n
        n = n - 1
    return result

result: int = factorial_while(5)
print("factorial_while(5)=", result)