def foo(x, y, a, b, c):
    return x + y + a + b + c

d = [1,2,3]
print(foo(19, 89, *d))