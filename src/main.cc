#include <cstdio>

#include "src/heap/heap.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/runtime/universe.h"

using namespace saauso::internal;

int main() {
  HandleScope scope;

  Universe::Genesis();

  Handle<PyObject> object1 = PyString::NewInstance("Hello World");
  Handle<PyObject> object2(PySmi::FromInt(1234));
  Handle<PyBoolean> object3(Universe::py_true_object_);
  Handle<PyBoolean> object4(Universe::py_false_object_);
  Handle<PyNone> object5(Universe::py_none_object_);
  Handle<PyObject> object6(PySmi::FromInt(3));

  Handle<PyObject> object7 = PyObject::Mul(object2, object6);

  Handle<PyList> list = PyList::NewInstance(1);
  PyList::Append(list, object1);
  PyList::Append(list, object2);
  PyList::Append(list, object3);
  PyList::Append(list, object4);
  PyList::Append(list, object5);
  PyList::Append(list, object6);
  PyList::Append(list, object7);

  for (auto i = 0; i < list->length(); ++i) {
    PyObject::Print(list->Get(i));
    std::putchar('\n');
  }

  std::puts("----------------------------");

  auto list2 = Handle<PyList>::Cast(PyObject::Add(list, list));
  for (auto i = 0; i < list2->length(); ++i) {
    PyObject::Print(list2->Get(i));
    std::putchar('\n');
  }

  Universe::heap_->DoGc();
}
