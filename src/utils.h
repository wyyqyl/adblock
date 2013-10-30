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

}  // namespace utils
}  // namespace adblock

#endif  // UTILS_H_
