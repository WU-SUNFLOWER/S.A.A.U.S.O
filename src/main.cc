#include <cstdio>

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

  Handle<PyObject> object7 = object2->Mul(object6);

  Handle<PyList> list = PyList::NewInstance(1);
  list->Append(object1);
  list->Append(object2);
  list->Append(object3);
  list->Append(object4);
  list->Append(object5);
  list->Append(object6);
  list->Append(object7);

  for (auto i = 0; i < list->length(); ++i) {
    list->Get(i)->Print();
    std::putchar('\n');
  }

  std::puts("----------------------------");

  auto list2 = Handle<PyList>::Cast(list->Add(list));
  for (auto i = 0; i < list2->length(); ++i) {
    list2->Get(i)->Print();
    std::putchar('\n');
  }
}
