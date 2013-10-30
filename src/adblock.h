#ifndef ADBLOCK_H_
#define ADBLOCK_H_

#include <boost/shared_ptr.hpp>

namespace adblock {

class AdBlock {
 public:
  virtual ~AdBlock() {}
};

typedef boost::shared_ptr<AdBlock> AdBlockPtr;

void CreateInstance(AdBlockPtr *adblock);

}  // namespace adblock

#endif  // ADBLOCK_H_
