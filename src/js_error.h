#ifndef JS_ERROR_H_
#define JS_ERROR_H_

#include <stdexcept>

namespace v8 {

class Isolate;
class TryCatch;

}  // namespace v8

namespace adblock {

class JsError : public std::runtime_error {
 public:
  JsError(v8::Isolate* isolate, v8::TryCatch* try_catch);
};

}  // namespace adblock

#endif  // JS_ERROR_H_
