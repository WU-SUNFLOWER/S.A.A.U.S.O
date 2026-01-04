#include <iostream>

#include "build/build_config.h"  // 引入我们的探测头文件
#include "build/buildflag.h"

void PlatformSpecificFunction(int x, int y) {
  // 这种写法优于 #ifdef，因为编译器会检查所有分支的语法
  if (BUILDFLAG(IS_WIN)) {
    std::cout << "[Logic] I am running on Windows!" << std::endl;
  }

  if (BUILDFLAG(IS_LINUX)) {
    std::cout << "[Logic] I am running on Linux!" << std::endl;
  }
}

void TestAsan() {
  int* ar = new int[10];
  ar[10086] = 123;
  std::cout << "unreachable!!!" << std::endl;
}

int main() {
  std::cout << "Hello, BuildFlags!" << std::endl;

  PlatformSpecificFunction(1, 2);

  // 你甚至可以直接打印宏的值
  std::cout << "Debug info: IS_WIN=" << BUILDFLAG(IS_WIN)
            << ", IS_LINUX=" << BUILDFLAG(IS_LINUX) << std::endl;

  TestAsan();

  return 0;
}
