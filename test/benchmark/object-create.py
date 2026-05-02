import time

class Point:
    def __init__(self, x, y):
        self.x = x
        self.y = y

def main():
    i = 0
    start_time = time.perf_counter()
    while i < 200000:
        point = Point(i, i + 1)
        i += 1
    end_time = time.perf_counter()
    elapsed = end_time - start_time
    print(elapsed)

main()