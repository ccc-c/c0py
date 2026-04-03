import math

def circle_area(r: float) -> float:
    return math.pi * r * r

result: float = circle_area(2.0)
print("circle_area(2.0)=", result)