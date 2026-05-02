import time

def generate_data(n):
    i = n
    data = []
    while i >= 0:
        data.append(i)
        i -= 1
    return data

def bubble_sort(arr):
    n = len(arr)
    i = 0
    while i < n - 1:
        swapped = False
        j = 0
        while j < n - i - 1:
            if arr[j] > arr[j + 1]:
                arr[j], arr[j + 1] = arr[j + 1], arr[j]
                swapped = True
            j += 1
        if not swapped:
            break
        i += 1
    return arr

def main():
    data = generate_data(10000)

    start_time = time.perf_counter()
    sorted_data = bubble_sort(data)
    end_time = time.perf_counter()

    elapsed = end_time - start_time
    print(elapsed)

main()