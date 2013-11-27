#ifndef ADBLOCK_IMPL_H_
#define ADBLOCK_IMPL_H_

#include "adblock.h"
#include "js_value.h"

namespace adblock {

class Environment;

class AdBlockImpl : public AdBlock {
 public:
  AdBlockImpl();
  ~AdBlockImpl();

  bool Init();

  std::string CheckFilterMatch(const std::string& location,
                               const std::string& type,
                               const std::string& document);
  std::string GetElementHidingSelectors(const std::string& domain);

 private:
  Environment* env_;
  bool is_first_run_;
  bool initialized_;
  // Hide placeholders of blocked elements
  bool collapse_;

  void InitDone(const JsValueList& args);
  void BlockingHit(const JsValueList& args);
};

}  // namespace adblock

#endif  // ADBLOCK_IMPL_H_
