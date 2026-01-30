def func():
    x = 2
    
    def say():
        print(x)

    return say

f = func()
f()

"""
code
   argcount 0
   posonlyargcount 0
   kwonlyargcount 0
   nlocals 0
   stacksize 2
   flags 0000
   code
      9700640084005a0002006500ab000000000000005a0102006501ab000000
      0000000001007901
  0           0 RESUME                   0

  1           2 LOAD_CONST               0 (<code object func at 0x000001AF2C576BF0, file "e:\MyProject\S.A.A.U.S.O\test\python312\closure1.py", line 1>)
              4 MAKE_FUNCTION            0
              6 STORE_NAME               0 (func)

  9           8 PUSH_NULL
             10 LOAD_NAME                0 (func)
             12 CALL                     0
             14 CACHE                    0 (counter: 0)
             16 CACHE                    0 (func_version: 0)
             18 CACHE                    0
             20 STORE_NAME               1 (f)

 10          22 PUSH_NULL
             24 LOAD_NAME                1 (f)
             26 CALL                     0
             28 CACHE                    0 (counter: 0)
             30 CACHE                    0 (func_version: 0)
             32 CACHE                    0
             34 POP_TOP
             36 RETURN_CONST             1 (None)
   consts
      code
         argcount 0
         posonlyargcount 0
         kwonlyargcount 0
         nlocals 1
         stacksize 2
         flags 0003
         code 8701970064018a0188016601640284087d007c005300
              0 MAKE_CELL                1 (x)

  1           2 RESUME                   0

  2           4 LOAD_CONST               1 (2)
              6 STORE_DEREF              1 (x)

  4           8 LOAD_CLOSURE             1 (x)
             10 BUILD_TUPLE              1
             12 LOAD_CONST               2 (<code object say at 0x000001AF2C518AB0, file "e:\MyProject\S.A.A.U.S.O\test\python312\closure1.py", line 4>)
             14 MAKE_FUNCTION            8 (closure)
             16 STORE_FAST               0 (say)

  7          18 LOAD_FAST                0 (say)
             20 RETURN_VALUE
         consts
            None
            2
            code
               argcount 0
               posonlyargcount 0
               kwonlyargcount 0
               nlocals 0
               stacksize 3
               flags 0013
               code 95019700740100000000000000008900ab0100000000000001007900
              0 COPY_FREE_VARS           1

  4           2 RESUME                   0

  5           4 LOAD_GLOBAL              1 (NULL + print)
              6 CACHE                    0 (counter: 0)
              8 CACHE                    0 (index: 0)
             10 CACHE                    0 (module_keys_version: 0)
             12 CACHE                    0 (builtin_keys_version: 0)
             14 LOAD_DEREF               0 (x)
             16 CALL                     1
             18 CACHE                    0 (counter: 0)
             20 CACHE                    0 (func_version: 0)
             22 CACHE                    0
             24 POP_TOP
             26 RETURN_CONST             0 (None)
               consts
                  None
               names ('print',)
               varnames ()
               freevars ('x',)
               cellvars ()
               filename 'e:\\MyProject\\S.A.A.U.S.O\\test\\python312\\closure1.py'
               name 'say'
               qualname 'func.<locals>.say'
               firstlineno 4
               linetable f88000dc080d88618d08
               lines [(0, 2, None), (2, 4, 4), (4, 28, 5)]
               positions [(None, None, None, None), (4, 4, 0, 0), (5, 5, 8, 13), (5, 5, 8, 13), (5, 5, 8, 13), (5, 5, 8, 13), (5, 5, 8, 13), (5, 5, 14, 15), (5, 5, 8, 16), (5, 5, 8, 16), (5, 5, 8, 16), (5, 5, 8, 16), (5, 5, 8, 16), (5, 5, 8, 16)]
               exceptiontable
e:\MyProject\S.A.A.U.S.O\tools\show_pyc.py:143: DeprecationWarning: co_lnotab is deprecated, use co_lines instead.
  if hasattr(code, "co_lnotab"):
               lnotab(deprecated) 0401
         names ()
         varnames ('say',)
         freevars ()
         cellvars ('x',)
         filename 'e:\\MyProject\\S.A.A.U.S.O\\test\\python312\\closure1.py'
         name 'func'
         qualname 'func'
         firstlineno 1
         linetable f88000d808098041f404010511f006000c0f804a
         lines [(0, 2, None), (2, 4, 1), (4, 8, 2), (8, 18, 4), (18, 22, 7)]
         positions [(None, None, None, None), (1, 1, 0, 0), (2, 2, 8, 9), (2, 2, 4, 5), (4, 5, 4, 16), (4, 5, 4, 16), (4, 5, 4, 16), (4, 5, 4, 16), (4, 5, 4, 16), (7, 7, 11, 14), (7, 7, 4, 14)]
         exceptiontable
e:\MyProject\S.A.A.U.S.O\tools\show_pyc.py:143: DeprecationWarning: co_lnotab is deprecated, use co_lines instead.
  if hasattr(code, "co_lnotab"):
         lnotab(deprecated) 040104020a03
      None
   names ('func', 'f')
   varnames ()
   freevars ()
   cellvars ()
   filename 'e:\\MyProject\\S.A.A.U.S.O\\test\\python312\\closure1.py'
   name '<module>'
   qualname '<module>'
   firstlineno 1
   linetable f003010101f20206010ff11000050983468001d900018503
   lines [(0, 2, 0), (2, 8, 1), (8, 22, 9), (22, 38, 10)]
   positions [(0, 1, 0, 0), (1, 7, 0, 14), (1, 7, 0, 14), (1, 7, 0, 14), (9, 9, 4, 8), (9, 9, 4, 8), (9, 9, 4, 10), (9, 9, 4, 10), (9, 9, 4, 10), (9, 9, 4, 10), (9, 9, 0, 1), (10, 10, 0, 1), (10, 10, 0, 1), (10, 10, 0, 3), (10, 10, 0, 3), (10, 10, 0, 3), (10, 10, 0, 3), (10, 10, 0, 3), (10, 10, 0, 3)]
   exceptiontable
e:\MyProject\S.A.A.U.S.O\tools\show_pyc.py:143: DeprecationWarning: co_lnotab is deprecated, use co_lines instead.
  if hasattr(code, "co_lnotab"):
   lnotab(deprecated) 00ff020106080e01
"""