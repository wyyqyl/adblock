#ifndef UTILS_H_
#define UTILS_H_

#include <string>

namespace adblock {
namespace utils {

inline std::string V8_STRING_TO_STD_STRING(const v8::Handle<v8::Value>& str) {
  v8::String::Utf8Value utf8(str);
  return std::string(*utf8, utf8.length());
}
#define V8_STRING_TO_STD_STRING adblock::utils::V8_STRING_TO_STD_STRING

inline v8::Local<v8::String> STD_STRING_TO_V8_STRING(v8::Isolate* isolate,
                                                     const std::string& str) {
  return v8::String::NewFromUtf8(isolate, str.c_str());
}
#define STD_STRING_TO_V8_STRING adblock::utils::STD_STRING_TO_V8_STRING

}  // namespace utils
}  // namespace adblock

#endif  // UTILS_H_
