def foo(a, b):
    return a / b

print(foo(b = 2, a = 6))

# code
#    argcount 0
#    posonlyargcount 0
#    kwonlyargcount 0
#    nlocals 0
#    stacksize 6
#    flags 0000
#    code
#       9700640084005a00020065010200650064016402ac03ab02000000000000
#       ab0100000000000001007904
#   0           0 RESUME                   0

#   1           2 LOAD_CONST               0 (<code object foo at 0x000002B4AC65D620, file "e:\MyProject\S.A.A.U.S.O\test\key_args.py", line 1>)
#               4 MAKE_FUNCTION            0
#               6 STORE_NAME               0 (foo)

#   4           8 PUSH_NULL
#              10 LOAD_NAME                1 (print)
#              12 PUSH_NULL
#              14 LOAD_NAME                0 (foo)
#              16 LOAD_CONST               1 (2)
#              18 LOAD_CONST               2 (6)
#              20 KW_NAMES                 3 (('b', 'a'))
#              22 CALL                     2
#              24 CACHE                    0 (counter: 0)
#              26 CACHE                    0 (func_version: 0)
#              28 CACHE                    0
#              30 CALL                     1
#              32 CACHE                    0 (counter: 0)
#              34 CACHE                    0 (func_version: 0)
#              36 CACHE                    0
#              38 POP_TOP
#              40 RETURN_CONST             4 (None)
#    consts
#       code
#          argcount 2
#          posonlyargcount 0
#          kwonlyargcount 0
#          nlocals 2
#          stacksize 2
#          flags 0003
#          code 97007c007c017a0b00005300
#   1           0 RESUME                   0

#   2           2 LOAD_FAST                0 (a)
#               4 LOAD_FAST                1 (b)
#               6 BINARY_OP               11 (/)
#               8 CACHE                    0 (counter: 0)
#              10 RETURN_VALUE
#          consts
#             None
#          names ()
#          varnames ('a', 'b')
#          freevars ()
#          cellvars ()