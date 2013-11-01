#ifndef LOG_SYSTEM_H_
#define LOG_SYSTEM_H_

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

namespace adblock {

class LogSystem {
 public:
  enum LogLevel {
    LOG_LEVEL_TRACE,
    LOG_LEVEL_LOG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
  };

  virtual ~LogSystem() {}
  virtual void operator()(LogLevel level,
                          const std::string& message,
                          const std::string& source) = 0;

 protected:
  boost::mutex mutex_;
};

typedef boost::shared_ptr<LogSystem> LogSystemPtr;

class DefaultLogSystem : public LogSystem {
 public:
  void operator()(LogLevel level,
                  const std::string& message,
                  const std::string& source);
};

}  // namespace adblock

#endif  // LOG_SYSTEM_H_
