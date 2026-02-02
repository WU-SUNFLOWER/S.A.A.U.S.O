class A(object):
    value = 1 

a = A() 
print(a.value)

"""
code
   argcount 0
   posonlyargcount 0
   kwonlyargcount 0
   nlocals 0
   stacksize 5
   flags 0000
   code
      9700020047006400840064016500ab030000000000005a0102006501ab00
      0000000000005a020200650365026a080000000000000000000000000000
      00000000ab0100000000000001007902
  0           0 RESUME                   0

  1           2 PUSH_NULL
              4 LOAD_BUILD_CLASS
              6 LOAD_CONST               0 (<code object A at 0x00000168A204D550, file "e:/MyProject/S.A.A.U.S.O/test/python312/build_class1.py", line 1>)
              8 MAKE_FUNCTION            0
             10 LOAD_CONST               1 ('A')
             12 LOAD_NAME                0 (object)
             14 CALL                     3
             16 CACHE                    0 (counter: 0)
             18 CACHE                    0 (func_version: 0)
             20 CACHE                    0
             22 STORE_NAME               1 (A)

  4          24 PUSH_NULL
             26 LOAD_NAME                1 (A)
             28 CALL                     0
             30 CACHE                    0 (counter: 0)
             32 CACHE                    0 (func_version: 0)
             34 CACHE                    0
             36 STORE_NAME               2 (a)

  5          38 PUSH_NULL
             40 LOAD_NAME                3 (print)
             42 LOAD_NAME                2 (a)
             44 LOAD_ATTR                8 (value)
             46 CACHE                    0 (counter: 0)
             48 CACHE                    0 (version: 0)
             50 CACHE                    0
             52 CACHE                    0 (keys_version: 0)
             54 CACHE                    0
             56 CACHE                    0 (descr: 0)
             58 CACHE                    0
             60 CACHE                    0
             62 CACHE                    0
             64 CALL                     1
             66 CACHE                    0 (counter: 0)
             68 CACHE                    0 (func_version: 0)
             70 CACHE                    0
             72 POP_TOP
             74 RETURN_CONST             2 (None)
   consts
      code
         argcount 0
         posonlyargcount 0
         kwonlyargcount 0
         nlocals 0
         stacksize 1
         flags 0000
         code 970065005a0164005a0264015a037902
  1           0 RESUME                   0
              2 LOAD_NAME                0 (__name__)
              4 STORE_NAME               1 (__module__)
              6 LOAD_CONST               0 ('A')
              8 STORE_NAME               2 (__qualname__)

  2          10 LOAD_CONST               1 (1)
             12 STORE_NAME               3 (value)
             14 RETURN_CONST             2 (None)
         consts
            'A'
            1
            None
         names ('__name__', '__module__', '__qualname__', 'value')
         varnames ()
         freevars ()
         cellvars ()
         filename 'e:/MyProject/S.A.A.U.S.O/test/python312/build_class1.py'
         name 'A'
         qualname 'A'
         firstlineno 1
         linetable 8400d80c0d8145
         lines [(0, 10, 1), (10, 16, 2)]
         positions [(1, 1, 0, 0), (1, 1, 0, 0), (1, 1, 0, 0), (1, 1, 0, 0), (1, 1, 0, 0), (2, 2, 12, 13), (2, 2, 4, 9), (2, 2, 4, 9)]
         exceptiontable
E:\MyProject\S.A.A.U.S.O\tools\show_pyc.py:143: DeprecationWarning: co_lnotab is deprecated, use co_lines instead.
  if hasattr(code, "co_lnotab"):
         lnotab(deprecated) 0a01
      'A'
      None
   names ('object', 'A', 'a', 'print', 'value')
   varnames ()
   freevars ()
   cellvars ()
   filename 'e:/MyProject/S.A.A.U.S.O/test/python312/build_class1.py'
   name '<module>'
   qualname '<module>'
   firstlineno 1
   linetable
      f003010101f40201010e8806f40001010ef10600050683438001d9000580
      6187678167850e
   lines [(0, 2, 0), (2, 24, 1), (24, 38, 4), (38, 76, 5)]
   positions [(0, 1, 0, 0), (1, 2, 0, 13), (1, 2, 0, 13), (1, 2, 0, 13), (1, 2, 0, 13), (1, 2, 0, 13), (1, 1, 8, 14), (1, 2, 0, 13), (1, 2, 0, 13), (1, 2, 0, 13), (1, 2, 0, 13), (1, 2, 0, 13), (4, 4, 4, 5), (4, 4, 4, 5), (4, 4, 4, 7), (4, 4, 4, 7), (4, 4, 4, 7), (4, 4, 4, 7), (4, 4, 0, 1), (5, 5, 0, 5), (5, 5, 0, 5), (5, 5, 6, 7), (5, 5, 6, 13), (5, 5, 6, 13), (5, 5, 6, 13), (5, 5, 6, 13), (5, 5, 6, 13), (5, 5, 6, 13), (5, 5, 6, 13), (5, 5, 6, 13), (5, 5, 6, 13), (5, 5, 6, 13), (5, 5, 0, 14), (5, 5, 0, 14), (5, 5, 0, 14), (5, 5, 0, 14), (5, 5, 0, 14), (5, 5, 0, 14)]
   exceptiontable
"""