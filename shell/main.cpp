#include "../src/adblock.h"
#include <stdexcept>
#include <iostream>
#include <Windows.h>
#include <vld.h>
#include <ctime>

#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "Ws2_32.lib")
#ifdef _DEBUG
#pragma comment(lib, "v8_snapshot-sd.lib")
#pragma comment(lib, "v8_base.ia32-sd.lib")
#pragma comment(lib, "icuuc-sd.lib")
#pragma comment(lib, "icui18n-sd.lib")
#pragma comment(lib, "libcurl-sd.lib")
#pragma comment(lib, "libeay32-sd.lib")
#pragma comment(lib, "ssleay32-sd.lib")
#pragma comment(lib, "zlib-sd.lib")
#else
#pragma comment(lib, "v8_snapshot-s.lib")
#pragma comment(lib, "v8_base.ia32-s.lib")
#pragma comment(lib, "icuuc-s.lib")
#pragma comment(lib, "icui18n-s.lib")
#pragma comment(lib, "libcurl-s.lib")
#pragma comment(lib, "libeay32-s.lib")
#pragma comment(lib, "ssleay32-s.lib")
#pragma comment(lib, "zlib-s.lib")
#endif

int main() {
  try {
    adblock::AdBlockPtr adblock;
    adblock::CreateInstance(&adblock);
    if (!adblock) {
      return -1;
    }

    time_t timer = time(nullptr);
    adblock->CheckFilterMatch(
        "http://pagead2.googlesyndication.com/pagead/show_ads.js", "SCRIPT",
        "http://bbs.pediy.com/");
    adblock->GetElementHidingSelectors("http://reddit.com/");
    std::cout << "Time elapsed: " << difftime(time(nullptr), timer)
              << std::endl;
  }
  catch (const std::exception& e) {
    OutputDebugStringA(e.what());
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
