#ifndef JS_OBJECT_H_
#define JS_OBJECT_H_

#include <v8.h>

namespace adblock {

class Environment;

namespace js_object {

void Setup(Environment* env);

namespace file_system_object {
v8::Local<v8::Object> Setup(Environment* env);
}  // namespace file_system_object

namespace web_request_object {
v8::Local<v8::Object> Setup(Environment* env);
}  // namespace web_request_object

namespace console_object {
v8::Local<v8::Object> Setup(Environment* env);
}  // namespace console_object

}  // namespace js_object
}  // namespace adblock

#endif  // JS_OBJECT_H_
