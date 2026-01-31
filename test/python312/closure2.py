def call_cnt(fn):
    cnt = 0
    def inner_func(*args):
        nonlocal cnt
        cnt += 1
        print(f"cnt={cnt}")
        return fn(*args)

    return inner_func

def add(a, b = 2): 
    return a + b 

f = call_cnt(add)
print(f(1, 2))
print(f(2, 3))