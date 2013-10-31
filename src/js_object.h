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

class IoThread : public Thread {
 public:
  IoThread(v8::Isolate* isolate, JsValuePtr callback)
      : Thread(isolate), callback_(callback) {
    file_system_ = env_->GetFileSystem();
  }

 protected:
  FileSystemPtr file_system_;
  JsValuePtr callback_;
};

class ReadThread : public IoThread {
 public:
  ReadThread(v8::Isolate* isolate, const std::string& path, JsValuePtr callback)
      : IoThread(isolate, callback), path_(path) {
  }
  void Run();

 private:
  std::string path_;
};

class WriteThread : public IoThread {
 public:
  WriteThread(v8::Isolate* isolate,
              const std::string& path,
              const std::string& data,
              JsValuePtr callback)
      : IoThread(isolate, callback), path_(path), data_(data) {
  }
  void Run();

 private:
  std::string path_;
  std::string data_;
};

class RemoveThread : public IoThread {
 public:
  RemoveThread(v8::Isolate* isolate,
               const std::string& path,
               JsValuePtr callback)
      : IoThread(isolate, callback), path_(path) {
  }
  void Run();

 private:
  std::string path_;
};

class MoveThread : public IoThread {
 public:
  MoveThread(v8::Isolate* isolate,
             const std::string& from,
             const std::string& to,
             JsValuePtr callback)
      : IoThread(isolate, callback), from_(from), to_(to) {
  }
  void Run();

 private:
  std::string from_;
  std::string to_;
};

class StatThread : public IoThread {
 public:
  StatThread(v8::Isolate* isolate, const std::string& path, JsValuePtr callback)
      : IoThread(isolate, callback), path_(path) {
  }
  void Run();

 private:
  std::string path_;
};

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
