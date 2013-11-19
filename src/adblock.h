#ifndef ADBLOCK_H_
#define ADBLOCK_H_

#include <boost/shared_ptr.hpp>
#include <string>

namespace adblock {

class AdBlock {
 public:
  virtual ~AdBlock() {}
  virtual std::string CheckFilterMatch(const std::string& location,
                                       const std::string& type,
                                       const std::string& document) = 0;
  virtual std::string GetElementHidingSelectors(const std::string& domain) = 0;
  virtual void AddSubscription(const std::string& url) = 0;
};

typedef boost::shared_ptr<AdBlock> AdBlockPtr;

void CreateInstance(AdBlockPtr* adblock);

}  // namespace adblock

#endif  // ADBLOCK_H_
