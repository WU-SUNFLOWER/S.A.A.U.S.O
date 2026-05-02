import time

def fib(n):
    if n < 2:
        return n
    return fib(n - 1) + fib(n - 2)

def main():
    start_time = time.perf_counter()
    result = fib(32)
    end_time = time.perf_counter()
    elapsed = end_time - start_time
    print(result)
    print(elapsed)

main()