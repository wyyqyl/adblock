#include "../src/adblock.h"
#include <stdexcept>
#include <iostream>
#include <Windows.h>
#include <vld.h>
#include <ctime>

int main() {
  try {
    adblock::AdBlockPtr adblock;
    adblock::CreateInstance(&adblock);
    if (!adblock) {
      return -1;
    }

    time_t timer = time(nullptr);
    adblock->CheckFilterMatch("http://pagead2.googlesyndication.com/pagead/show_ads.js", "SCRIPT", "http://bbs.pediy.com/");
    adblock->GetElementHidingSelectors("http://reddit.com/");
    std::cout << "Time elapsed: " << difftime(time(nullptr), timer) << std::endl;
  } catch (const std::exception& e) {
    OutputDebugStringA(e.what());
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
