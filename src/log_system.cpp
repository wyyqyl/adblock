#include "log_system.h"
#include <sstream>
#ifdef WIN32
#include <Windows.h>
#endif  // WIN32

namespace adblock {

void DefaultLogSystem::operator()(LogLevel level, const std::string& message,
                                  const std::string& source) {
  boost::mutex::scoped_lock lock(mutex_);
  std::stringstream log;

  log << source;
  switch (level) {
    case LOG_LEVEL_TRACE:
      log << "Traceback:" << std::endl;
      break;
    case LOG_LEVEL_LOG:
      log << " ";
      break;
    case LOG_LEVEL_INFO:
      log << "[Info] ";
      break;
    case LOG_LEVEL_WARN:
      log << "[Warning] ";
      break;
    case LOG_LEVEL_ERROR:
      log << "[Error] ";
      break;
  }
  log << message << std::endl;
#ifdef WIN32
  OutputDebugStringA(log.str().c_str());
#endif  // WIN32
  std::cerr << log.str();
}

}  // namespace adblock
