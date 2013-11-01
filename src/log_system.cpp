#include "log_system.h"

namespace adblock {

void DefaultLogSystem::operator()(LogLevel level,
                                  const std::string& message,
                                  const std::string& source) {
  boost::mutex::scoped_lock lock(mutex_);

  std::cerr << source;
  switch (level) {
    case LOG_LEVEL_TRACE:
      std::cerr << "Traceback:" << std::endl;
      break;
    case LOG_LEVEL_LOG:
      std::cerr << " ";
      break;
    case LOG_LEVEL_INFO:
      std::cerr << "[Info] ";
      break;
    case LOG_LEVEL_WARN:
      std::cerr << "[Warning] ";
      break;
    case LOG_LEVEL_ERROR:
      std::cerr << "[Error] ";
      break;
  }
  std::cerr << message << std::endl;
}

}  // namespace adblock

