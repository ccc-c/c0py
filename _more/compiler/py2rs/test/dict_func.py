def get_value(d: dict, key: str) -> int:
    return d[key]

d = {"a": 10, "b": 20, "c": 30}
result: int = get_value(d, "b")
print("get_value(b)=", result)