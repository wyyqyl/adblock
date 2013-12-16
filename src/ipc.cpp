#include "ipc.h"
#include <glog/logging.h>

namespace adblock {

namespace ipc = boost::interprocess;
namespace pt = boost::posix_time;

AdblockConfig::AdblockConfig() : adblock_control_(nullptr), segment_(nullptr) {}

AdblockConfig::~AdblockConfig() {
  if (segment_ != nullptr) {
    delete segment_;
    segment_ = nullptr;
  }
}

bool AdblockConfig::Init() {
  // Init shared memory
  try {
    if (!segment_) {
      segment_ =
          new ipc::managed_shared_memory(ipc::open_only, "MySharedMemory");
    }
    if (segment_) {
      auto res = segment_->find<AdblockControl>("AdblockControl");
      if (res.second == 1 && res.first != nullptr) {
        adblock_control_ = res.first;
        return true;
      }
    }
    return false;
  }
  catch (const ipc::interprocess_exception& e) {
    LOG(ERROR) << "[AdblockConfig::Init()] " << e.what() << std::endl;
    return false;
  }
}

bool AdblockConfig::block_ads() {
  if (!adblock_control_) {
    Init();
  }
  if (adblock_control_) {
    return adblock_control_->block_ads;
  }
  return false;
}

bool AdblockConfig::block_malware() {
  if (!adblock_control_) {
    Init();
  }
  if (adblock_control_) {
    return adblock_control_->block_malware;
  }
  return false;
}

bool AdblockConfig::dont_track_me() {
  if (!adblock_control_) {
    Init();
  }
  if (adblock_control_) {
    return adblock_control_->dont_track_me;
  }
  return false;
}

AdblockSender::AdblockSender() : queue_(nullptr) {}

AdblockSender::~AdblockSender() {
  if (queue_ != nullptr) {
    delete queue_;
    queue_ = nullptr;
  }
}

void AdblockSender::Send(const std::string& msg) {
  try {
    if (!queue_) {
      queue_ = new ipc::message_queue(ipc::open_only, "message_queue");
    }
    pt::ptime abs_time(pt::microsec_clock::universal_time() +
                       pt::time_duration(0, 0, 1));
    queue_->timed_send(msg.data(), msg.size(), 0, abs_time);
  }
  catch (const ipc::interprocess_exception& e) {
    LOG(ERROR) << "[AdblockSender::Send] " << e.what() << std::endl;
    return;
  }
  catch (const std::bad_alloc&) {
    LOG(ERROR) << "Failed to allocate memory when creating message_queue object"
               << std::endl;
    return;
  }
}

}  // namespace adblock
