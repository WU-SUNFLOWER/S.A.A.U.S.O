def sum(*args):
    t = 0
    for i in args:
        t += i

    return t

print(sum(1, 2, 3, 4))

# code
#    argcount 0
#    posonlyargcount 0
#    kwonlyargcount 0
#    nlocals 0
#    stacksize 8
#    flags 0000
#    code
#       9700640084005a0002006501020065006401640264036404ab0400000000
#       0000ab0100000000000001007905
#   0           0 RESUME                   0

#   1           2 LOAD_CONST               0 (<code object sum at 0x0000019CABDF15C0, file "e:\MyProject\S.A.A.U.S.O\test\python312\extend_pos_args.py", line 1>)
#               4 MAKE_FUNCTION            0
#               6 STORE_NAME               0 (sum)

#   8           8 PUSH_NULL
#              10 LOAD_NAME                1 (print)
#              12 PUSH_NULL
#              14 LOAD_NAME                0 (sum)
#              16 LOAD_CONST               1 (1)
#              18 LOAD_CONST               2 (2)
#              20 LOAD_CONST               3 (3)
#              22 LOAD_CONST               4 (4)
#              24 CALL                     4
#              26 CACHE                    0 (counter: 0)
#              28 CACHE                    0 (func_version: 0)
#              30 CACHE                    0
#              32 CALL                     1
#              34 CACHE                    0 (counter: 0)
#              36 CACHE                    0 (func_version: 0)
#              38 CACHE                    0
#              40 POP_TOP
#              42 RETURN_CONST             5 (None)
#    consts
#       code
#          argcount 0
#          posonlyargcount 0
#          kwonlyargcount 0
#          nlocals 3
#          stacksize 3
#          flags 0007
#          code
#             970064017d017c0044005d0700007d027c017c027a0d00007d018c090400
#             7c015300
#   1           0 RESUME                   0

#   2           2 LOAD_CONST               1 (0)
#               4 STORE_FAST               1 (t)

#   3           6 LOAD_FAST                0 (args)
#               8 GET_ITER
#         >>   10 FOR_ITER                 7 (to 28)
#              12 CACHE                    0 (counter: 0)
#              14 STORE_FAST               2 (i)

#   4          16 LOAD_FAST                1 (t)
#              18 LOAD_FAST                2 (i)
#              20 BINARY_OP               13 (+=)
#              22 CACHE                    0 (counter: 0)
#              24 STORE_FAST               1 (t)
#              26 JUMP_BACKWARD            9 (to 10)

#   3     >>   28 END_FOR

#   6          30 LOAD_FAST                1 (t)
#              32 RETURN_VALUE
#          consts
#             None
#             0
#          names ()
#          varnames ('args', 't', 'i')
#          freevars ()
#          cellvars ()