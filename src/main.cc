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
  Handle<PyObject> object2(PySmi::FromInt(1234));
  object2->Print();
}
