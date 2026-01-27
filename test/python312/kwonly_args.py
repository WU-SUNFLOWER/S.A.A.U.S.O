def example(a, b, *args, c, d=42):
    print(a, b, args, c, d)

example(1, 2, 3, 4, 5, 6)


# code
#    argcount 0
#    posonlyargcount 0
#    kwonlyargcount 0
#    nlocals 0
#    stacksize 10
#    flags 0000
#    code
#       9700640064019c01640284025a0002006500640364046405640664076408
#       6409640aac0bab080000000000000100790c
#   0           0 RESUME                   0

#   1           2 LOAD_CONST               0 (42)
#               4 LOAD_CONST               1 (('d',))
#               6 BUILD_CONST_KEY_MAP      1
#               8 LOAD_CONST               2 (<code object example at 0x000002143EDF15C0, file "e:\MyProject\S.A.A.U.S.O\test\python312\kwonly_args.py", line 1>)
#              10 MAKE_FUNCTION            2 (kwdefaults)
#              12 STORE_NAME               0 (example)

#   4          14 PUSH_NULL
#              16 LOAD_NAME                0 (example)
#              18 LOAD_CONST               3 (1)
#              20 LOAD_CONST               4 (2)
#              22 LOAD_CONST               5 (3)
#              24 LOAD_CONST               6 (4)
#              26 LOAD_CONST               7 (5)
#              28 LOAD_CONST               8 (6)
#              30 LOAD_CONST               9 (8)
#              32 LOAD_CONST              10 (9)
#              34 KW_NAMES                11 (('c', 'd'))
#              36 CALL                     8
#              38 CACHE                    0 (counter: 0)
#              40 CACHE                    0 (func_version: 0)
#              42 CACHE                    0
#              44 POP_TOP
#              46 RETURN_CONST            12 (None)
#    consts
#       42
#       ('d',)
#       code
#          argcount 2
#          posonlyargcount 0
#          kwonlyargcount 2
#          nlocals 5
#          stacksize 7
#          flags 0007
#          code
#             9700740100000000000000007c007c017c047c027c03ab05000000000000
#             01007900
#   1           0 RESUME                   0

#   2           2 LOAD_GLOBAL              1 (NULL + print)
#               4 CACHE                    0 (counter: 0)
#               6 CACHE                    0 (index: 0)
#               8 CACHE                    0 (module_keys_version: 0)
#              10 CACHE                    0 (builtin_keys_version: 0)
#              12 LOAD_FAST                0 (a)
#              14 LOAD_FAST                1 (b)
#              16 LOAD_FAST                4 (args)
#              18 LOAD_FAST                2 (c)
#              20 LOAD_FAST                3 (d)
#              22 CALL                     5
#              24 CACHE                    0 (counter: 0)
#              26 CACHE                    0 (func_version: 0)
#              28 CACHE                    0
#              30 POP_TOP
#              32 RETURN_CONST             0 (None)
#          consts
#             None
#          names ('print',)
#          varnames ('a', 'b', 'c', 'd', 'args')
#          freevars ()
#          cellvars ()
#          filename 'e:\\MyProject\\S.A.A.U.S.O\\test\\python312\\kwonly_args.py'
#          name 'example'
#          qualname 'example'
#          firstlineno 1
#          linetable 8000dc040988218851900490619811d5041b
#          lines [(0, 2, 1), (2, 34, 2)]
#          positions [(1, 1, 0, 0), (2, 2, 4, 9), (2, 2, 4, 9), (2, 2, 4, 9), (2, 2, 4, 9), (2, 2, 4, 9), (2, 2, 10, 11), (2, 2, 13, 14), (2, 2, 16, 20), (2, 2, 22, 23), (2, 2, 25, 26), (2, 2, 4, 27), (2, 2, 4, 27), (2, 2, 4, 27), (2, 2, 4, 27), (2, 2, 4, 27), (2, 2, 4, 27)]
#          exceptiontable
# e:\MyProject\S.A.A.U.S.O\tools\show_pyc.py:143: DeprecationWarning: co_lnotab is deprecated, use co_lines instead.
#   if hasattr(code, "co_lnotab"):
#          lnotab(deprecated) 0201
#       1
#       2
#       3
#       4
#       5
#       6
#       8
#       9
#       ('c', 'd')
#       None
#    names ('example',)
#    varnames ()
#    freevars ()
#    cellvars ()