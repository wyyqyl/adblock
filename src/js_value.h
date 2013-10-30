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

class JsValue {
 public:
  /**
  * Calling this constructor is not safe in another thread,
  * you MUST enter the context scope before calling it or call the function
  * JsValue(const v8::Handle<v8::Context>& context,
  * const v8::Handle<v8::Value>& value);
  * @see JsValue(const v8::Handle<v8::Context>& context, const v8::Handle<v8::Value>& value);
  */
  JsValue(v8::Isolate* isolate, const v8::Handle<v8::Value>& value);

  /**
   * @see JsValue(v8::Isolate* isolate, const v8::Handle<v8::Value>& value);
   */
  JsValue(const v8::Handle<v8::Context>& context,
          const v8::Handle<v8::Value>& value);

  JsValuePtr Call(const JsValueList& args = JsValueList(),
                  JsValuePtr recv = JsValuePtr());

  inline bool BooleanValue() const {
    return value_->BooleanValue();
  }
  inline bool IsObject() const {
    return value_->IsObject();
  }

 private:
  JsData<v8::Value> value_;
  Environment* env_;
};

}  // namespace adblock

#endif  // JS_VALUE_H_
