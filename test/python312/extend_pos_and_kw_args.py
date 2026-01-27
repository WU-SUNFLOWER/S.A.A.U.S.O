def calc(a, b, *args, **kwargs):
    coeff = kwargs.get("coeff")

    if coeff is None:
        return 0

    t = a + b
    for i in args:
        t += i

    return coeff * t

print(calc(1, 2, 3, 4, coeff = 2))




# flags 0 (0b00)
# pyc_type timestamp-based
# mtime 1769452790 (Tue Jan 27 02:39:50 2026)
# source_size 211
# code
#    argcount 0
#    posonlyargcount 0
#    kwonlyargcount 0
#    nlocals 0
#    stacksize 9
#    flags 0000
#    code
#       9700640084005a00020065010200650064016402640364046402ac05ab05
#       000000000000ab0100000000000001007906
#   0           0 RESUME                   0

#   1           2 LOAD_CONST               0 (<code object calc at 0x0000022432F5DA10, file "e:\MyProject\S.A.A.U.S.O\test\python312\extend_pos_and_kw_args.py", line 1>)
#               4 MAKE_FUNCTION            0
#               6 STORE_NAME               0 (calc)

#  13           8 PUSH_NULL
#              10 LOAD_NAME                1 (print)
#              12 PUSH_NULL
#              14 LOAD_NAME                0 (calc)
#              16 LOAD_CONST               1 (1)
#              18 LOAD_CONST               2 (2)
#              20 LOAD_CONST               3 (3)
#              22 LOAD_CONST               4 (4)
#              24 LOAD_CONST               2 (2)
#              26 KW_NAMES                 5 (('coeff',))
#              28 CALL                     5
#              30 CACHE                    0 (counter: 0)
#              32 CACHE                    0 (func_version: 0)
#              34 CACHE                    0
#              36 CALL                     1
#              38 CACHE                    0 (counter: 0)
#              40 CACHE                    0 (func_version: 0)
#              42 CACHE                    0
#              44 POP_TOP
#              46 RETURN_CONST             6 (None)
#    consts
#       code
#          argcount 2
#          posonlyargcount 0
#          kwonlyargcount 0
#          nlocals 7
#          stacksize 3
#          flags 000f
#          code
#             97007c036a010000000000000000000000000000000000006401ab010000
#             000000007d047c04800179027c007c017a0000007d057c0244005d070000
#             7d067c057c067a0d00007d058c0904007c047c057a0500005300
#   1           0 RESUME                   0

#   2           2 LOAD_FAST                3 (kwargs)
#               4 LOAD_ATTR                1 (NULL|self + get)
#               6 CACHE                    0 (counter: 0)
#               8 CACHE                    0 (version: 0)
#              10 CACHE                    0
#              12 CACHE                    0 (keys_version: 0)
#              14 CACHE                    0
#              16 CACHE                    0 (descr: 0)
#              18 CACHE                    0
#              20 CACHE                    0
#              22 CACHE                    0
#              24 LOAD_CONST               1 ('coeff')
#              26 CALL                     1
#              28 CACHE                    0 (counter: 0)
#              30 CACHE                    0 (func_version: 0)
#              32 CACHE                    0
#              34 STORE_FAST               4 (coeff)

#   4          36 LOAD_FAST                4 (coeff)
#              38 POP_JUMP_IF_NOT_NONE     1 (to 42)

#   5          40 RETURN_CONST             2 (0)

#   7     >>   42 LOAD_FAST                0 (a)
#              44 LOAD_FAST                1 (b)
#              46 BINARY_OP                0 (+)
#              48 CACHE                    0 (counter: 0)
#              50 STORE_FAST               5 (t)

#   8          52 LOAD_FAST                2 (args)
#              54 GET_ITER
#         >>   56 FOR_ITER                 7 (to 74)
#              58 CACHE                    0 (counter: 0)
#              60 STORE_FAST               6 (i)

#   9          62 LOAD_FAST                5 (t)
#              64 LOAD_FAST                6 (i)
#              66 BINARY_OP               13 (+=)
#              68 CACHE                    0 (counter: 0)
#              70 STORE_FAST               5 (t)
#              72 JUMP_BACKWARD            9 (to 56)

#   8     >>   74 END_FOR

#  11          76 LOAD_FAST                4 (coeff)
#              78 LOAD_FAST                5 (t)
#              80 BINARY_OP                5 (*)
#              82 CACHE                    0 (counter: 0)
#              84 RETURN_VALUE
#          consts
#             None
#             'coeff'
#             0
#          names ('get',)
#          varnames ('a', 'b', 'args', 'kwargs', 'coeff', 't', 'i')
#          freevars ()
#          cellvars ()
#          filename 'e:\\MyProject\\S.A.A.U.S.O\\test\\python312\\extend_pos_and_kw_args.py'
#          name 'calc'
#          qualname 'calc'
#          firstlineno 1
#          linetable
#             8000d80c128f4a894a9077d30c1f8045e0070c807dd80f10e00809884189
#             058041db0d118801d80809885189068901f003000e12f006000c11903189
#             39d00414
#          lines [(0, 2, 1), (2, 36, 2), (36, 40, 4), (40, 42, 5), (42, 52, 7), (52, 62, 8), (62, 74, 9), (74, 76, 8), (76, 86, 11)]
#          positions [(1, 1, 0, 0), (2, 2, 12, 18), (2, 2, 12, 22), (2, 2, 12, 22), (2, 2, 12, 22), (2, 2, 12, 22), (2, 2, 12, 22), (2, 2, 12, 22), (2, 2, 12, 22), (2, 2, 12, 22), (2, 2, 12, 22), (2, 2, 12, 22), (2, 2, 23, 30), (2, 2, 12, 31), (2, 2, 12, 31), (2, 2, 12, 31), (2, 2, 12, 31), (2, 2, 4, 9), (4, 4, 7, 12), (4, 4, 7, 20), (5, 5, 15, 16), (7, 7, 8, 9), (7, 7, 12, 13), (7, 7, 8, 13), (7, 7, 8, 13), (7, 7, 4, 5), (8, 8, 13, 17), (8, 8, 13, 17), (8, 8, 13, 17), (8, 8, 13, 17), (8, 8, 8, 9), (9, 9, 8, 9), (9, 9, 13, 14), (9, 9, 8, 14), (9, 9, 8, 14), (9, 9, 8, 9), (9, 9, 8, 9), (8, 8, 13, 17), (11, 11, 11, 16), (11, 11, 19, 20), (11, 11, 11, 20), (11, 11, 11, 20), (11, 11, 4, 20)]
#          exceptiontable
# e:\MyProject\S.A.A.U.S.O\tools\show_pyc.py:143: DeprecationWarning: co_lnotab is deprecated, use co_lines instead.
#   if hasattr(code, "co_lnotab"):
#          lnotab(deprecated) 02012202040102020a010a010cff0203
#       1
#       2
#       3
#       4
#       ('coeff',)
#       None
#    names ('calc', 'print')
#    varnames ()
#    freevars ()
#    cellvars ()
#    filename 'e:\\MyProject\\S.A.A.U.S.O\\test\\python312\\extend_pos_and_kw_args.py'
#    name '<module>'
#    qualname '<module>'
#    firstlineno 1
#    linetable
#       f003010101f2020a0115f118000106816488318861901190419871d40621
#       d50022
#    lines [(0, 2, 0), (2, 8, 1), (8, 48, 13)]
#    positions [(0, 1, 0, 0), (1, 11, 0, 20), (1, 11, 0, 20), (1, 11, 0, 20), (13, 13, 0, 5), (13, 13, 0, 5), (13, 13, 6, 10), (13, 13, 6, 10), (13, 13, 11, 12), (13, 13, 14, 15), (13, 13, 17, 18), (13, 13, 20, 21), (13, 13, 31, 32), (13, 13, 6, 33), (13, 13, 6, 33), (13, 13, 6, 33), (13, 13, 6, 33), (13, 13, 6, 33), (13, 13, 0, 34), (13, 13, 0, 34), (13, 13, 0, 34), (13, 13, 0, 34), (13, 13, 0, 34), (13, 13, 0, 34)]
#    exceptiontable
# e:\MyProject\S.A.A.U.S.O\tools\show_pyc.py:143: DeprecationWarning: co_lnotab is deprecated, use co_lines instead.
#   if hasattr(code, "co_lnotab"):
#    lnotab(deprecated) 00ff0201060c