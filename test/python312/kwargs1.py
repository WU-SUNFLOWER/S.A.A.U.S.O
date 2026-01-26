def sum(*args):
    t = 0
    for i in args:
        t += i

    return t

print(sum(1, 2, 3, 4))