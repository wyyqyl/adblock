#include "ipc.h"

namespace adblock {

namespace ipc = boost::interprocess;
namespace pt = boost::posix_time;

AdblockConfig::AdblockConfig() : adblock_control_(nullptr) {}

bool AdblockConfig::Init() {
  // Init shared memory
  try {
    ipc::managed_shared_memory segment(ipc::open_only, "MySharedMemory");
    auto res = segment.find<AdblockControl>("AdblockControl");
    if (res.second == 1 && res.first != nullptr) {
      adblock_control_ = res.first;
      return true;
    }
    return false;
  }
  catch (const ipc::interprocess_exception& e) {
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

AdblockConfig::~AdblockConfig() {}

AdblockSender::AdblockSender() : queue_(nullptr) {}

AdblockSender::~AdblockSender() {
  if (queue_ != nullptr) {
    delete queue_;
    queue_ = nullptr;
  }
}

void AdblockSender::Send(const std::string& msg) {
  if (!queue_) {
    try {
      queue_ = new ipc::message_queue(ipc::open_only, "message_queue");
    }
    catch (const ipc::interprocess_exception& e) {
      return;
    }
    catch (const std::bad_alloc& e) {
      return;
    }
  }
  pt::ptime abs_time(pt::microsec_clock::universal_time() +
                     pt::time_duration(0, 0, 1));
  queue_->timed_send(msg.data(), msg.size(), 0, abs_time);
}

}  // namespace adblock
