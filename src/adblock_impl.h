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

  bool block_ads();
  bool block_malware();
  bool dont_track_me();

  std::string CheckFilterMatch(const std::string& location,
                               const std::string& type,
                               const std::string& document);
  std::string GetElementHidingSelectors(const std::string& domain);
  bool IsWhitelisted(const std::string& url, const std::string& parent_url,
                     const std::string& type);
  void ToggleEnabled(const std::string& url, bool enabled);
  std::string GenerateCSSContent();
  void Report(const std::string& type, const std::string& documentUrl,
              const std::string& url, const std::string& rule);

 private:
  Environment* env_;
  bool is_first_run_;
  bool initialized_;
  AdblockConfig config_;
  AdblockSender sender_;

  std::string GetCurrentProcessName();
  void InitDone(const JsValueList& args);
};

}  // namespace adblock

#endif  // ADBLOCK_IMPL_H_
