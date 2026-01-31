def foo(x, y):
    def bar():
        print(x)
        def baz():
            print(y)
        return baz
    return bar

bar_instance = foo(12, 25)
baz_instance = bar_instance()
baz_instance()