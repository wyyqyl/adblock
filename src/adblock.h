#ifndef ADBLOCK_H_
#define ADBLOCK_H_

#include <boost/shared_ptr.hpp>
#include <string>

namespace adblock {

typedef enum _IPC_CMD {
  UNKNOWN = 0,
  ENABLE_ADBLOCK,
  DISABLE_ADBLOCK,
  ADD_DOMAIN_TO_EXCEPTION_LIST,
  REMOVE_DOMAIN_FROM_EXCEPTION_LIST
} IPC_CMD;

class AdBlock {
 public:
  virtual ~AdBlock() {}
  virtual bool enabled() = 0;
  virtual void set_enabled(bool enabled) = 0;
  virtual std::string CheckFilterMatch(const std::string& location,
                                       const std::string& type,
                                       const std::string& document) = 0;
  virtual std::string GetElementHidingSelectors(const std::string& domain) = 0;
  virtual bool IsWhitelisted(const std::string& url,
                             const std::string& parent_url,
                             const std::string& type) = 0;
  virtual void ToggleEnabled(const std::string& url, bool enabled) = 0;
  virtual std::string GenerateCSSContent() = 0;
};

typedef boost::shared_ptr<AdBlock> AdBlockPtr;

void CreateInstance(AdBlockPtr* adblock);

}  // namespace adblock

#endif  // ADBLOCK_H_
