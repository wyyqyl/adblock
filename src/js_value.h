#ifndef JS_VALUE_H_
#define JS_VALUE_H_

#include "js_data.h"

#include <boost/shared_ptr.hpp>
#include <vector>

namespace adblock {

class Environment;
class JsValue;
typedef boost::shared_ptr<JsValue> JsValuePtr;
typedef std::vector<JsValuePtr> JsValueList;
typedef std::vector<v8::Handle<v8::Value>> CallParams;

class JsValue {
 public:
  JsValue(v8::Isolate* isolate, const v8::Handle<v8::Value>& value);

  JsValuePtr Call(const JsValueList& args, Environment* env);
  JsValuePtr Call(const CallParams& args = CallParams(),
                  Environment* env = nullptr);

  inline bool BooleanValue() const {
    return value_->BooleanValue();
  }
  inline bool IsObject() const {
    return value_->IsObject();
  }

 private:
  JsData<v8::Value> value_;
};

}  // namespace adblock

#endif  // JS_VALUE_H_
