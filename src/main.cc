#include <iostream>
#include <string>

#include "src/heap/heap.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/runtime/universe.h"

using namespace saauso::internal;

int main() {
  Universe::Genesis();

  {
    HandleScope scope;
    constexpr int kLength = 5;

    Handle<PyList> list1 = PyList::NewInstance(kLength);

    for (auto i = 0; i < kLength; ++i) {
      HandleScope scope;
      Handle<PyString> elem = PyString::NewInstance(std::to_string(i).c_str());
      PyList::Append(list1, elem);
    }

    Address a1 = (*list1).ptr();
    Universe::heap_->CollectGarbage();
    Address a2 = (*list1).ptr();

    std::cout << "a1=" << a1 << ", a2=" << a2 << std::endl;
  }

  Universe::Destroy();
}
