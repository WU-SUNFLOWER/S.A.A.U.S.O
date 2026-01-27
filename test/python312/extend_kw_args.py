def foo(**kwargs):
    for k, v in kwargs.items():
        print(k)
        print(v)

foo(a = 1, b = 2)

# code
#    argcount 0
#    posonlyargcount 0
#    kwonlyargcount 0                                                                                                                                                                              (8, 8, 6, 21), (8, 8, 6, 21), (8, 8, 6, 21), (8, 8, 6, 21), (8,
#    nlocals 0
#    stacksize 4
#    flags 0000 
#    code 9700640084005a000200650064016402ac03ab0200000000000001007904
#   0           0 RESUME                   0

#   1           2 LOAD_CONST               0 (<code object foo at 0x000001BE7B45DA10, file "e:\MyProject\S.A.A.U.S.O\test\python312\extend_kw_args.py", line 1>)
#               4 MAKE_FUNCTION            0
#               6 STORE_NAME               0 (foo)

#   6           8 PUSH_NULL
#              10 LOAD_NAME                0 (foo)
#              12 LOAD_CONST               1 (1)
#              14 LOAD_CONST               2 (2)
#              16 KW_NAMES                 3 (('a', 'b'))
#              18 CALL                     2
#              20 CACHE                    0 (counter: 0)
#              22 CACHE                    0 (func_version: 0)
#              24 CACHE                    0
#              26 POP_TOP
#              28 RETURN_CONST             4 (None)
#    consts
#       code
#          argcount 0
#          posonlyargcount 0
#          kwonlyargcount 0
#          nlocals 3
#          stacksize 4
#          flags 000b
#          code
#             97007c006a01000000000000000000000000000000000000ab0000000000
#             000044005d1b00005c0200007d017d02740300000000000000007c01ab01
#             0000000000000100740300000000000000007c02ab010000000000000100
#             8c1d04007900
#   1           0 RESUME                   0

#   2           2 LOAD_FAST                0 (kwargs)
#               4 LOAD_ATTR                1 (NULL|self + items)
#               6 CACHE                    0 (counter: 0)
#               8 CACHE                    0 (version: 0)
#              10 CACHE                    0
#              12 CACHE                    0 (keys_version: 0)
#              14 CACHE                    0
#              16 CACHE                    0 (descr: 0)
#              18 CACHE                    0
#              20 CACHE                    0
#              22 CACHE                    0
#              24 CALL                     0
#              26 CACHE                    0 (counter: 0)
#              28 CACHE                    0 (func_version: 0)
#              30 CACHE                    0
#              32 GET_ITER
#         >>   34 FOR_ITER                27 (to 92)
#              36 CACHE                    0 (counter: 0)
#              38 UNPACK_SEQUENCE          2
#              40 CACHE                    0 (counter: 0)
#              42 STORE_FAST               1 (k)
#              44 STORE_FAST               2 (v)

#   3          46 LOAD_GLOBAL              3 (NULL + print)
#              48 CACHE                    0 (counter: 0)
#              50 CACHE                    0 (index: 0)
#              52 CACHE                    0 (module_keys_version: 0)
#              54 CACHE                    0 (builtin_keys_version: 0)
#              56 LOAD_FAST                1 (k)
#              58 CALL                     1
#              60 CACHE                    0 (counter: 0)
#              62 CACHE                    0 (func_version: 0)
#              64 CACHE                    0
#              66 POP_TOP

#   4          68 LOAD_GLOBAL              3 (NULL + print)
#              70 CACHE                    0 (counter: 0)
#              72 CACHE                    0 (index: 0)
#              74 CACHE                    0 (module_keys_version: 0)
#              76 CACHE                    0 (builtin_keys_version: 0)
#              78 LOAD_FAST                2 (v)
#              80 CALL                     1
#              82 CACHE                    0 (counter: 0)
#              84 CACHE                    0 (func_version: 0)
#              86 CACHE                    0
#              88 POP_TOP
#              90 JUMP_BACKWARD           29 (to 34)

#   2     >>   92 END_FOR
#              94 RETURN_CONST             0 (None)
#          consts
#             None
#          names ('items', 'print')
#          varnames ('kwargs', 'k', 'v')
#          freevars ()
#          cellvars ()