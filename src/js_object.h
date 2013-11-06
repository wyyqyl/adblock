#ifndef JS_OBJECT_H_
#define JS_OBJECT_H_

#include "env.h"
#include "utils.h"

namespace adblock {

class Thread {
 public:
  explicit Thread(v8::Isolate* isolate)
      : env_(Environment::GetCurrent(isolate)) {
  }
  virtual ~Thread() {}

  virtual boost::thread* Start() {
    boost::thread t = boost::thread(&Thread::Run, this);
    t.detach();
    return nullptr;
  }

 protected:
  Environment* env_;

 private:
  virtual void Run() = 0;
};

namespace js_object {

inline JsValueList CONVERT_ARGUMENTS(
    const v8::FunctionCallbackInfo<v8::Value>& args,
    int start = 0,
    int end = -1) {
  JsValueList arguments;
  if (start >= args.Length()) {
    return arguments;
  }
  if (end == -1 || end > args.Length()) {
    end = args.Length();
  }
  for (int idx = start; idx < end; ++idx) {
    arguments.push_back(JsValuePtr(new JsValue(args.GetIsolate(), args[idx])));
  }
  return arguments;
}
#define CONVERT_ARGUMENTS adblock::js_object::CONVERT_ARGUMENTS

class TimeoutThread : public Thread {
 public:
  explicit TimeoutThread(const v8::FunctionCallbackInfo<v8::Value>& args)
      : Thread(args.GetIsolate()),
        delay_(args[1]->Int32Value()),
        func_(new JsValue(args.GetIsolate(), args[0])),
        args_(CONVERT_ARGUMENTS(args, 2)) {
  }

  boost::thread* Start();

 private:
  int delay_;
  JsValuePtr func_;
  JsValueList args_;

  void Run(boost::thread* thread);
  void Run() {}
};

void Setup(Environment* env);

namespace file_system_object {

class IoThread : public Thread {
 public:
  IoThread(v8::Isolate* isolate, const v8::Handle<v8::Value>& value)
      : Thread(isolate),
        file_system_(env_->GetFileSystem()),
        callback_(new JsValue(isolate, value)) {
  }

 protected:
  FileSystemPtr file_system_;
  JsValuePtr callback_;
};

class ReadThread : public IoThread {
 public:
  explicit ReadThread(const v8::FunctionCallbackInfo<v8::Value>& args)
      : IoThread(args.GetIsolate(), args[1]),
        path_(V8_STRING_TO_STD_STRING(args[0]->ToString())) {
  }

 private:
  std::string path_;

  void Run();
};

class WriteThread : public IoThread {
 public:
  explicit WriteThread(const v8::FunctionCallbackInfo<v8::Value>& args)
      : IoThread(args.GetIsolate(), args[2]),
        path_(V8_STRING_TO_STD_STRING(args[0]->ToString())),
        data_(V8_STRING_TO_STD_STRING(args[1]->ToString())) {
  }

 private:
  std::string path_;
  std::string data_;

  void Run();
};

class RemoveThread : public IoThread {
 public:
  explicit RemoveThread(const v8::FunctionCallbackInfo<v8::Value>& args)
      : IoThread(args.GetIsolate(), args[1]),
        path_(V8_STRING_TO_STD_STRING(args[0]->ToString())) {
  }

 private:
  std::string path_;

  void Run();
};

class MoveThread : public IoThread {
 public:
  explicit MoveThread(const v8::FunctionCallbackInfo<v8::Value>& args)
      : IoThread(args.GetIsolate(), args[2]),
        from_(V8_STRING_TO_STD_STRING(args[0]->ToString())),
        to_(V8_STRING_TO_STD_STRING(args[1]->ToString())) {
  }

 private:
  std::string from_;
  std::string to_;

  void Run();
};

class StatThread : public IoThread {
 public:
  explicit StatThread(const v8::FunctionCallbackInfo<v8::Value>& args)
      : IoThread(args.GetIsolate(), args[1]),
        path_(V8_STRING_TO_STD_STRING(args[0]->ToString())) {
  }

 private:
  std::string path_;

  void Run();
};

v8::Local<v8::Object> Setup(Environment* env);
}  // namespace file_system_object

namespace web_request_object {

class WebRequestThread : public Thread {
 public:
  explicit WebRequestThread(const v8::FunctionCallbackInfo<v8::Value>& args)
      : Thread(args.GetIsolate()),
        web_request_(env_->GetWebRequest()),
        url_(V8_STRING_TO_STD_STRING(args[0]->ToString())),
        callback_(new JsValue(args.GetIsolate(), args[2])) {
    auto obj = v8::Handle<v8::Object>::Cast<v8::Value>(args[1]);
    v8::Local<v8::Array> properties = obj->GetOwnPropertyNames();
    for (uint32_t idx = 0; idx < properties->Length(); ++idx) {
      v8::Local<v8::Value> property = properties->Get(idx);
      v8::Local<v8::Value> value = obj->Get(property);
      std::string property_name = V8_STRING_TO_STD_STRING(property->ToString());
      std::string value_name = V8_STRING_TO_STD_STRING(value->ToString());
      headers_.push_back(std::make_pair(property_name, value_name));
    }
  }

 private:
  WebRequestPtr web_request_;
  std::string url_;
  WebRequest::HeaderList headers_;
  JsValuePtr callback_;

  void Run();
};

v8::Local<v8::Object> Setup(Environment* env);
}  // namespace web_request_object

namespace console_object {
v8::Local<v8::Object> Setup(Environment* env);
}  // namespace console_object

}  // namespace js_object
}  // namespace adblock

#endif  // JS_OBJECT_H_
