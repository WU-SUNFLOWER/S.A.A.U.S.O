def foo(x, y, a, b, c):
    return x + y + a + b + c

d = {"a": 1, "b": 2, "c": 3}
print(foo(19, 89, **d))