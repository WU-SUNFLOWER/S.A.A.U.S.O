class MyList(list):
    pass


class MyDict(dict):
    pass


class MyTuple(tuple):
    pass


class MyStr(str):
    pass


l = MyList()
l.x = 1
print(l.x)

d = MyDict()
d.x = 2
print(d.x)

t = MyTuple()
t.x = 3
print(t.x)

s = MyStr()
s.x = 4
print(s.x)
