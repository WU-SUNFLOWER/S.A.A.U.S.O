# test_default.py
def foo(a, b = 1, c = 2):
    return a + b + c

print(foo(10)) # 13
print(foo(100, 20, 30)) # 150