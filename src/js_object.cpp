#include "js_object.h"
#include "js_error.h"
#include "utils.h"

namespace adblock {
namespace js_object {

// Used to be a macro, hence the uppercase name.
template <typename T>
inline void ADB_SET_METHOD(const T& recv,
                           const char* name,
                           v8::FunctionCallback callback) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(callback);
  recv->Set(v8::String::NewFromUtf8(isolate, name), t->GetFunction());
}
#define ADB_SET_METHOD adblock::js_object::ADB_SET_METHOD

// Used to be a macro, hence the uppercase name.
template <typename T>
inline void ADB_SET_OBJECT(const T& recv,
                           const char* name,
                           const v8::Local<v8::Object>& obj) {
  recv->Set(v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), name), obj);
}
#define ADB_SET_OBJECT adblock::js_object::ADB_SET_OBJECT

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

#define ADB_THROW_EXCEPTION(isolate, str)                                     \
  do {                                                                        \
    (isolate)->ThrowException(v8::String::NewFromUtf8((isolate), (str)));     \
    return;                                                                   \
  } while (0)


void TimeoutThread::Run() {
  boost::this_thread::sleep(boost::posix_time::milliseconds(delay_));
  func_->Call(args_);
  delete this;
}

void SetTimeoutCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  if (args.Length() < 2) {
    ADB_THROW_EXCEPTION(isolate, "setTimeout requires at least 2 parameters!");
  }
  if (!args[0]->IsFunction()) {
    ADB_THROW_EXCEPTION(isolate,
                        "First argument to setTimeout must be a function!");
  }

  JsValuePtr func = JsValuePtr(new JsValue(isolate, args[0]));
  int delay = args[1]->Int32Value();
  JsValueList arguments = CONVERT_ARGUMENTS(args, 2);
  Thread* thread = new TimeoutThread(isolate, delay, func, arguments);
  thread->Start();
}

void TriggerCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  if (args.Length() < 1) {
    ADB_THROW_EXCEPTION(isolate, "trigger expects at least one parameter");
  }

  JsValueList arguments = CONVERT_ARGUMENTS(args, 1);
  Environment* env = Environment::GetCurrent(isolate);
  env->TriggerEvent(V8_STRING_TO_STD_STRING(args[0]->ToString()), arguments);
}

void Setup(Environment* env) {
  auto global = env->context()->Global();
  ADB_SET_METHOD(global, "setTimeout",  SetTimeoutCallback);
  ADB_SET_METHOD(global, "trigger",     TriggerCallback);
  ADB_SET_OBJECT(global, "fileSystem",  file_system_object::Setup(env));
  ADB_SET_OBJECT(global, "webRequest",  web_request_object::Setup(env));
  ADB_SET_OBJECT(global, "console",     console_object::Setup(env));
}

namespace file_system_object {

void ReadCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
}

void WriteCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
}

void RemoveCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
}

void MoveCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
}

void StatCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
}

void ResolveCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
}

v8::Local<v8::Object> Setup(Environment* env) {
  v8::EscapableHandleScope handle_scope(env->isolate());
  auto obj = v8::Object::New();
  ADB_SET_METHOD(obj, "read",     ReadCallback);
  ADB_SET_METHOD(obj, "write",    WriteCallback);
  ADB_SET_METHOD(obj, "remove",   RemoveCallback);
  ADB_SET_METHOD(obj, "move",     MoveCallback);
  ADB_SET_METHOD(obj, "stat",     StatCallback);
  ADB_SET_METHOD(obj, "resolve",  ResolveCallback);
  return handle_scope.Escape(obj);
}

}  // namespace file_system_object

namespace web_request_object {

void GetCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
}

v8::Local<v8::Object> Setup(Environment* env) {
  v8::EscapableHandleScope handle_scope(env->isolate());
  auto obj = v8::Object::New();
  ADB_SET_METHOD(obj, "get", GetCallback);
  return handle_scope.Escape(obj);
}

}  // namespace web_request_object

namespace console_object {

void LogCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
}

void InfoCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
}

void WarnCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
}

void ErrorCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
}

void TraceCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
}

v8::Local<v8::Object> Setup(Environment* env) {
  v8::EscapableHandleScope handle_scope(env->isolate());
  auto obj = v8::Object::New();
  ADB_SET_METHOD(obj, "log",    LogCallback);
  ADB_SET_METHOD(obj, "info",   InfoCallback);
  ADB_SET_METHOD(obj, "warn",   WarnCallback);
  ADB_SET_METHOD(obj, "error",  ErrorCallback);
  ADB_SET_METHOD(obj, "trace",  TraceCallback);
  return handle_scope.Escape(obj);
}

}  // namespace console_object

}  // namespace js_object
}  // namespace adblock
