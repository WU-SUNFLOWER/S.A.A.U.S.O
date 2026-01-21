// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <span>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "include/saauso.h"
#include "src/build/build_config.h"
#include "src/build/buildflag.h"
#include "src/code/cpython312-pyc-compiler.h"
#include "src/code/cpython312-pyc-file-parser.h"
#include "src/handles/global-handles.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/isolate.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_LINUX)
extern "C" const char* __lsan_default_suppressions() {
  return "leak:PyUnicode_New\n";
}
#endif

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = "saauso_test.py";

Handle<PyCodeObject> CompileScript(std::string_view source,
                                   std::string_view file_name) {
  std::vector<uint8_t> pyc =
      CompilePythonSourceToPycBytes312(source, file_name);

  CPython312PycFileParser parser(
      std::span<const uint8_t>(pyc.data(), pyc.size()));
  return parser.Parse();
}

#define PRINT_TO_EXPECT_LIST(x) PyList::Append(expected_printv_result, x)

#define EXPECT_PY_EQUAL(x, y) EXPECT_TRUE(IsPyTrue(PyObject::Equal(x, y)))

#define PY_TRUE handle(isolate_->py_true_object())
#define PY_FALSE handle(isolate_->py_false_object())

}  // namespace

class BasicInterpreterTest : public testing::Test {
 protected:
  static void SetUpTestSuite() {
    saauso::Saauso::Initialize();
    isolate_ = Isolate::New();
    isolate_->Enter();

    // 向interpreter中注入回调函数
    HandleScope scope;
    Handle<PyString> func_name = PyString::NewInstance("print");
    PyDict::Put(isolate_->interpreter()->builtins(), func_name,
                PyFunction::NewInstance(&Native_PrintV, func_name));
  }

  static void TearDownTestSuite() {
    // 在isolate销毁前手工释放掉global指针，
    // 避免测试夹具析构时（此时虚拟机堆已经被销毁）才释放
    printv_result_.Reset();
    isolate_->Exit();
    Isolate::Dispose(isolate_);
    isolate_ = nullptr;
    saauso::Saauso::Dispose();
    FinalizeEmbeddedPython312Runtime();
  }

  void SetUp() override {
    HandleScope scope;
    // 初始化printv_result_
    printv_result_ = PyList::NewInstance();
  }

  static Handle<PyObject> Native_PrintV(Handle<PyTuple> args,
                                        Handle<PyDict> kwargs) {
    for (auto i = 0; i < args->length(); ++i) {
      HandleScope scope;
      PyList::Append(printv_result_.Get(), args->Get(i));
    }
    return handle(Isolate::Current()->py_none_object());
  }

  static void CompareResultWithExpected(Handle<PyList> expected) {
    HandleScope scope;
    Handle<PyList> printv_result = printv_result_.Get();

    EXPECT_EQ(printv_result->length(), expected->length());

    for (auto i = 0; i < printv_result->length(); ++i) {
      EXPECT_PY_EQUAL(printv_result->Get(i), expected->Get(i));
    }
  }

  static Isolate* isolate_;

  static Global<PyList> printv_result_;
};

Isolate* BasicInterpreterTest::isolate_ = nullptr;
Global<PyList> BasicInterpreterTest::printv_result_;

TEST_F(BasicInterpreterTest, HelloWorld) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
print("Hello World")
)";

  isolate_->interpreter()->Run(CompileScript(kSource, kTestFileName));

  // 检查打印结果
  auto printv_result = printv_result_.Get();
  ASSERT_EQ(printv_result->length(), 1);
  EXPECT_PY_EQUAL(printv_result->Get(0), PyString::NewInstance("Hello World"));
}

TEST_F(BasicInterpreterTest, MakeAndCallFunction) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def foo():
  print("foo")
print("before")
foo()
print("after")
)";

  isolate_->interpreter()->Run(CompileScript(kSource, kTestFileName));

  // 检查打印结果
  auto expected_printv_result = PyList::NewInstance();
  PyList::Append(expected_printv_result, PyString::NewInstance("before"));
  PyList::Append(expected_printv_result, PyString::NewInstance("foo"));
  PyList::Append(expected_printv_result, PyString::NewInstance("after"));

  CompareResultWithExpected(expected_printv_result);
}

TEST_F(BasicInterpreterTest, CompareOpTest) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
# group1
print(1 < 3)
print(1 > 3)

# group2
print(1 <= 3)
print(1 >= 3)

# group3
print(3 <= 3)
print(3 >= 3)

# group4
print(6 == 6)
print(6 != 6)
print(1 != 6)
)";

  isolate_->interpreter()->Run(CompileScript(kSource, kTestFileName));

  // 检查打印结果
  auto expected_printv_result = PyList::NewInstance();
  // group 1
  PyList::Append(expected_printv_result, PY_TRUE);
  PyList::Append(expected_printv_result, PY_FALSE);

  // group 2
  PyList::Append(expected_printv_result, PY_TRUE);
  PyList::Append(expected_printv_result, PY_FALSE);

  // group 3
  PyList::Append(expected_printv_result, PY_TRUE);
  PyList::Append(expected_printv_result, PY_TRUE);

  // group 4
  PyList::Append(expected_printv_result, PY_TRUE);
  PyList::Append(expected_printv_result, PY_FALSE);
  PyList::Append(expected_printv_result, PY_TRUE);

  CompareResultWithExpected(expected_printv_result);
}

TEST_F(BasicInterpreterTest, IsOpTest) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
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
print(d is x) # d是拼接得到的，它与x指向的就不是同一个对象了
print(d is not x)
)";

  isolate_->interpreter()->Run(CompileScript(kSource, kTestFileName));

  // 检查打印结果
  auto expected_printv_result = PyList::NewInstance();
  // group 1
  PyList::Append(expected_printv_result, PY_TRUE);
  PyList::Append(expected_printv_result, PY_TRUE);
  PyList::Append(expected_printv_result, PY_TRUE);

  // // group 2
  PyList::Append(expected_printv_result, PY_TRUE);
  PyList::Append(expected_printv_result, PY_FALSE);
  PyList::Append(expected_printv_result, PY_TRUE);

  CompareResultWithExpected(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ContainsOpTest) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
x = "Hello SAAUSO World"
y = "USO"
z = "Python"
print(y in x)
print(z in x)
print(x in z)

print(y not in x)
print(z not in x)
print(x not in z)
)";

  isolate_->interpreter()->Run(CompileScript(kSource, kTestFileName));

  // 检查打印结果
  auto expected_printv_result = PyList::NewInstance();
  PyList::Append(expected_printv_result, PY_TRUE);
  PyList::Append(expected_printv_result, PY_FALSE);
  PyList::Append(expected_printv_result, PY_FALSE);

  PyList::Append(expected_printv_result, PY_FALSE);
  PyList::Append(expected_printv_result, PY_TRUE);
  PyList::Append(expected_printv_result, PY_TRUE);

  CompareResultWithExpected(expected_printv_result);
}

TEST_F(BasicInterpreterTest, IfStatmentTest) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
x = 4
y = 2

if x > y:
  print(y - x)
else:
  print(x + y)

if x < y:
  print(1)
elif x == y:
  print(2)
elif x > y:
  print(3)
else:
  print(4)

if None:
  print(True)
else:
  print(False)
)";

  isolate_->interpreter()->Run(CompileScript(kSource, kTestFileName));

  // 检查打印结果
  auto expected_printv_result = PyList::NewInstance();
  PyList::Append(expected_printv_result, PyFloat::NewInstance(-2));
  PyList::Append(expected_printv_result, handle(PySmi::FromInt(3)));
  PyList::Append(expected_printv_result, handle(isolate_->py_false_object()));

  CompareResultWithExpected(expected_printv_result);
}

TEST_F(BasicInterpreterTest, WhileStatmentTest) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
a = 1 
b = 0 
i = 0 

print(a)
print(b)
print(i)

while i < 10: 
    print(a)
    t = a 
    a = a + b 
    b = t 
    i = i + 1 
)";

  isolate_->interpreter()->Run(CompileScript(kSource, kTestFileName));

  // 检查打印结果
  auto expected_printv_result = PyList::NewInstance();

  int a = 1, b = 0, i = 0;

  PRINT_TO_EXPECT_LIST(handle(PySmi::FromInt(a)));
  PRINT_TO_EXPECT_LIST(handle(PySmi::FromInt(b)));
  PRINT_TO_EXPECT_LIST(handle(PySmi::FromInt(i)));

  while (i < 10) {
    PRINT_TO_EXPECT_LIST(handle(PySmi::FromInt(a)));
    int t = a;
    a = a + b;
    b = t;
    i = i + 1;
  }

  CompareResultWithExpected(expected_printv_result);
}

TEST_F(BasicInterpreterTest, WhileWithBreakTest) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
i = 0
while i < 5:
    print(i)
    if i == 3:
        break
    i = i + 1
print(i)
)";

  isolate_->interpreter()->Run(CompileScript(kSource, kTestFileName));

  // 检查打印结果
  auto expected_printv_result = PyList::NewInstance();

  int i = 0;
  while (i < 5) {
    PRINT_TO_EXPECT_LIST(handle(PySmi::FromInt(i)));
    if (i == 3) {
      break;
    }
    i = i + 1;
  }
  PRINT_TO_EXPECT_LIST(handle(PySmi::FromInt(i)));

  CompareResultWithExpected(expected_printv_result);
}

TEST_F(BasicInterpreterTest, WhileWithContinueTest) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
i = 0
while i < 10:
  i = i + 1
  if i < 5:
    continue
  print(i)
)";

  isolate_->interpreter()->Run(CompileScript(kSource, kTestFileName));

  auto expected_printv_result = PyList::NewInstance();

  int i = 0;
  while (i < 10) {
    i = i + 1;
    if (i < 5) {
      continue;
    }
    PRINT_TO_EXPECT_LIST(handle(PySmi::FromInt(i)));
  }

  CompareResultWithExpected(expected_printv_result);
}

TEST_F(BasicInterpreterTest, WhileStatmentTest2) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
i = 0
j = 0
while j < 3:
    i = 0
    while i < 10:
        if i == 3:
            break
        i = i + 1
    j = j + 1
print(i)
print(j)
)";

  isolate_->interpreter()->Run(CompileScript(kSource, kTestFileName));

  auto expected_printv_result = PyList::NewInstance();

  int i = 0, j = 0;
  while (j < 3) {
    i = 0;
    while (i < 10) {
      if (i == 3) {
        break;
      }
      i = i + 1;
    }
    j = j + 1;
  }

  PRINT_TO_EXPECT_LIST(handle(PySmi::FromInt(i)));
  PRINT_TO_EXPECT_LIST(handle(PySmi::FromInt(j)));
  CompareResultWithExpected(expected_printv_result);
}

TEST_F(BasicInterpreterTest, FunctionWithArgs) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def add(a, b): 
    a += 233
    return a + b 

print(add(1, 2))
)";

  isolate_->interpreter()->Run(CompileScript(kSource, kTestFileName));

  auto expected_printv_result = PyList::NewInstance();

  PRINT_TO_EXPECT_LIST(handle(PySmi::FromInt(236)));
  CompareResultWithExpected(expected_printv_result);
}

}  // namespace saauso::internal
