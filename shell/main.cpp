#include "../src/adblock.h"
#include <stdexcept>
#include <iostream>
#include <Windows.h>
#include <vld.h>

int main() {
  try {
    adblock::AdBlockPtr adblock;
    adblock::CreateInstance(&adblock);
    getchar();
  } catch (const std::exception& e) {
    OutputDebugStringA(e.what());
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
