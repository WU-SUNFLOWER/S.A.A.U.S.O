x = "Hello World"
y = x
z = "Hello World"

# group1
# 现代python有优化机制, x、y、z实际上会指向同一个对象
print(x is y)
print(x is z)
print(y is z)

a = "Hello"
b = " "
c = "World"
d = a + b + c
# group2
print(d == x)
print(d is x)

d += "ooop"