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

  FilterPtr CheckFilterMatch(const std::string& location,
                             const std::string& type,
                             const std::string& document);

 private:
  Environment* env_;
  bool is_first_run_;
  // Hide placeholders of blocked elements
  bool collapse_;

  void InitDone(const JsValueList& args);
};

}  // namespace adblock

#endif  // ADBLOCK_IMPL_H_
