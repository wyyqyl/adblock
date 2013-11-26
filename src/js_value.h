#ifndef JS_VALUE_H_
#define JS_VALUE_H_

#include "js_data.h"

#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>

namespace adblock {

class Environment;
class JsValue;
typedef boost::shared_ptr<JsValue> JsValuePtr;
typedef std::vector<JsValuePtr> JsValueList;
typedef std::vector<v8::Handle<v8::Value>> CallParams;

class JsValue {
 public:
  JsValue(v8::Isolate* isolate, const v8::Handle<v8::Value>& value);

  v8::Local<v8::Value> Call(const JsValueList& args, Environment* env);
  v8::Local<v8::Value> Call(const CallParams& args = CallParams(),
                            Environment* env = nullptr);

  inline v8::Local<v8::Value> ToV8Value() const { return value_; }
  inline bool BooleanValue() const { return value_->BooleanValue(); }
  inline bool IsObject() const { return value_->IsObject(); }
  std::string ToStdString() const;

 private:
  JsData<v8::Value> value_;
};

}  // namespace adblock

#endif  // JS_VALUE_H_
