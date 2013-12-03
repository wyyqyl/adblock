#ifndef ADBLOCK_IMPL_H_
#define ADBLOCK_IMPL_H_

#include "adblock.h"
#include "js_value.h"
#include "ipc.h"

namespace adblock {

class Environment;

class AdBlockImpl : public AdBlock {
 public:
  AdBlockImpl();
  ~AdBlockImpl();

  bool Init();

  bool enabled();
  void set_enabled(bool enabled);

  std::string CheckFilterMatch(const std::string& location,
                               const std::string& type,
                               const std::string& document);
  std::string GetElementHidingSelectors(const std::string& domain);
  bool IsWhitelisted(const std::string& url, const std::string& parent_url,
                     const std::string& type);
  void ToggleEnabled(const std::string& url, bool enabled);

 private:
  Environment* env_;
  bool is_first_run_;
  bool initialized_;
  bool enabled_;
  IPCServer ipc_server_;
  IPCClient ipc_client_;

  void InitDone(const JsValueList& args);
  void BlockingHit(const JsValueList& args);
};

}  // namespace adblock

#endif  // ADBLOCK_IMPL_H_
