#ifndef JS_OBJECT_H_
#define JS_OBJECT_H_

#include "env.h"
#include <boost/thread/thread.hpp>

namespace adblock {

class Thread {
 public:
  explicit Thread(v8::Isolate* isolate)
      : env_(Environment::GetCurrent(isolate)) {
  }
  virtual ~Thread() {
  }
  void Start() {
    thread_ = boost::thread(&Thread::Run, this);
    thread_.detach();
  }
  virtual void Run() = 0;

 protected:
  Environment* env_;
  boost::thread thread_;
};

namespace js_object {

class TimeoutThread : public Thread {
 public:
  TimeoutThread(v8::Isolate* isolate,
                int delay,
                JsValuePtr func,
                const JsValueList& args)
      : Thread(isolate), delay_(delay), func_(func), args_(args) {
  }
  void Run();

 private:
  int delay_;
  JsValuePtr func_;
  JsValueList args_;
};

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
