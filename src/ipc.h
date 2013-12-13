#ifndef IPC_H_
#define IPC_H_

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

namespace adblock {

typedef struct _AdblockControl {
  bool block_ads;
  bool block_malware;
  bool dont_track_me;
} AdblockControl;

class AdblockConfig {
 public:
  AdblockConfig();
  ~AdblockConfig();

  bool block_ads();
  bool block_malware();
  bool dont_track_me();

 private:
  AdblockControl* adblock_control_;

  bool Init();
};

class AdblockSender {
 public:
  AdblockSender();
  ~AdblockSender();
  void Send(const std::string& msg);

 private:
  boost::interprocess::message_queue* queue_;
};

}  // namespace adblock

#endif  // IPC_H_
