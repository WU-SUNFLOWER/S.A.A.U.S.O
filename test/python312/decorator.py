def wrapper(fn):
    return fn

@wrapper
def add(a, b): 
    return a + b 

"""
magic 0xcb0d0d0a
flags 0 (0b00)
pyc_type timestamp-based
mtime 1769877720 (Sun Feb  1 00:42:00 2026)
source_size 74
code
   argcount 0
   posonlyargcount 0
   kwonlyargcount 0
   nlocals 0
   stacksize 2
   flags 0000
   code 9700640084005a00650064018400ab000000000000005a017902
  0           0 RESUME                   0

  1           2 LOAD_CONST               0 (<code object wrapper at 0x00000197CA10D550, file "e:/MyProject/S.A.A.U.S.O/test/python312/decorator.py", line 1>)
              4 MAKE_FUNCTION            0
              6 STORE_NAME               0 (wrapper)

  4           8 LOAD_NAME                0 (wrapper)

  5          10 LOAD_CONST               1 (<code object add at 0x00000197CA10D620, file "e:/MyProject/S.A.A.U.S.O/test/python312/decorator.py", line 4>)
             12 MAKE_FUNCTION            0

  4          14 CALL                     0
             16 CACHE                    0 (counter: 0)
             18 CACHE                    0 (func_version: 0)
             20 CACHE                    0

  5          22 STORE_NAME               1 (add)
             24 RETURN_CONST             2 (None)
   consts
      code
         argcount 1
         posonlyargcount 0
         kwonlyargcount 0
         nlocals 1
         stacksize 1
         flags 0003
         code 97007c005300
  1           0 RESUME                   0

  2           2 LOAD_FAST                0 (fn)
              4 RETURN_VALUE
         consts
            None
         names ()
         varnames ('fn',)
         freevars ()
         cellvars ()
         filename 'e:/MyProject/S.A.A.U.S.O/test/python312/decorator.py'
         name 'wrapper'
         qualname 'wrapper'
         firstlineno 1
         linetable 8000d80b0d8049
         lines [(0, 2, 1), (2, 6, 2)]
         positions [(1, 1, 0, 0), (2, 2, 11, 13), (2, 2, 4, 13)]
         exceptiontable
E:\MyProject\S.A.A.U.S.O\tools\show_pyc.py:143: DeprecationWarning: co_lnotab is deprecated, use co_lines instead.
  if hasattr(code, "co_lnotab"):
         lnotab(deprecated) 0201
      code
         argcount 2
         posonlyargcount 0
         kwonlyargcount 0
         nlocals 2
         stacksize 2
         flags 0003
         code 97007c007c017a0000005300
  4           0 RESUME                   0

  6           2 LOAD_FAST                0 (a)
              4 LOAD_FAST                1 (b)
              6 BINARY_OP                0 (+)
              8 CACHE                    0 (counter: 0)
             10 RETURN_VALUE
         consts
            None
         names ()
         varnames ('a', 'b')
         freevars ()
         cellvars ()
         filename 'e:/MyProject/S.A.A.U.S.O/test/python312/decorator.py'
         name 'add'
         qualname 'add'
         firstlineno 4
         linetable 8000e00b0c88718935804c
         lines [(0, 2, 4), (2, 12, 6)]
         positions [(4, 4, 0, 0), (6, 6, 11, 12), (6, 6, 15, 16), (6, 6, 11, 16), (6, 6, 11, 16), (6, 6, 4, 16)]
         exceptiontable
E:\MyProject\S.A.A.U.S.O\tools\show_pyc.py:143: DeprecationWarning: co_lnotab is deprecated, use co_lines instead.
  if hasattr(code, "co_lnotab"):
         lnotab(deprecated) 0202
      None
   names ('wrapper', 'add')
   varnames ()
   freevars ()
   cellvars ()
   filename 'e:/MyProject/S.A.A.U.S.O/test/python312/decorator.py'
   name '<module>'
   qualname '<module>'
   firstlineno 1
   linetable f003010101f20201010ef006000209f102010111f303000209f102010111
   lines [(0, 2, 0), (2, 8, 1), (8, 10, 4), (10, 14, 5), (14, 22, 4), (22, 26, 5)]
   positions [(0, 1, 0, 0), (1, 2, 0, 13), (1, 2, 0, 13), (1, 2, 0, 13), (4, 4, 1, 8), (5, 6, 0, 16), (5, 6, 0, 16), (4, 4, 1, 8), (4, 4, 1, 8), (4, 4, 1, 8), (4, 4, 1, 8), (5, 6, 0, 16), (5, 6, 0, 16)]
   exceptiontable
E:\MyProject\S.A.A.U.S.O\tools\show_pyc.py:143: DeprecationWarning: co_lnotab is deprecated, use co_lines instead.
  if hasattr(code, "co_lnotab"):
   lnotab(deprecated) 00ff02010603020104ff0801
"""