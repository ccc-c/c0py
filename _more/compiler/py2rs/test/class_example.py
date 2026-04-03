# Test with class and method

class Counter:
    def __init__(self):
        self.count = 0
    
    def increment(self):
        self.count += 1
        return self.count
    
    def get_count(self):
        return self.count

c = Counter()
c.increment()
c.increment()
print("count:", c.get_count())