#include "../src/adblock.h"
#include <stdexcept>
#include <iostream>
#include <Windows.h>
#include <vld.h>

int main() {
  try {
    adblock::AdBlockPtr adblock;
    adblock::CreateInstance(&adblock);
    if (!adblock) {
      return -1;
    }
    adblock::FilterPtr filter = adblock->CheckFilterMatch("http://pagead2.googlesyndication.com/pagead/show_ads.js", "SCRIPT", "http://bbs.pediy.com/");
    adblock::Filter::Type type = filter->type();
    bool collapse = filter->collapse();
  } catch (const std::exception& e) {
    OutputDebugStringA(e.what());
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
