import time

class Counter:
    def __init__(self, base):
        self.base = base

    def step(self, value):
        return self.base + value

def build_objects():
    i = 0
    result = []
    while i < 1000:
        result.append(Counter(i))
        i += 1
    return result

def main():
    objects = build_objects()
    outer = 0
    total = 0
    start_time = time.perf_counter()
    while outer < 200:
        inner = 0
        while inner < len(objects):
            total = total + objects[inner].step(inner)
            inner += 1
        outer += 1
    end_time = time.perf_counter()
    elapsed = end_time - start_time
    print(total)
    print(elapsed)

main()