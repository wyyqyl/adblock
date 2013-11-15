#ifndef ADBLOCK_H_
#define ADBLOCK_H_

#include <boost/shared_ptr.hpp>

namespace adblock {

class Filter {
 public:
  enum Type {
    TYPE_BLOCKING,
    TYPE_EXCEPTION,
    TYPE_ELEMHIDE,
    TYPE_ELEMHIDE_EXCEPTION,
    TYPE_COMMENT,
    TYPE_INVALID
  };

  explicit Filter(Type type = TYPE_INVALID)
      : type_(type),
        collapse_(true) {
  }

  inline Type type() const {
    return type_;
  }

  inline bool collapse() const {
    return collapse_;
  }
  inline void set_collapse(bool collapse) {
    collapse_ = collapse;
  }

 private:
  Type type_;
  // only meaningful when type_ == TYPE_BLOCKING
  bool collapse_;
};

typedef boost::shared_ptr<Filter> FilterPtr;


class AdBlock {
 public:
  virtual ~AdBlock() {}
  virtual FilterPtr CheckFilterMatch(const std::string& location,
                                     const std::string& type,
                                     const std::string& document) = 0;
};

typedef boost::shared_ptr<AdBlock> AdBlockPtr;

void CreateInstance(AdBlockPtr *adblock);

}  // namespace adblock

#endif  // ADBLOCK_H_
