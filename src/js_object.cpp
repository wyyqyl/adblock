#include "js_object.h"
#include "js_error.h"
#include "utils.h"

#include <boost/filesystem/operations.hpp>

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

#define SETUP_THREAD_CONTEXT(env)                                             \
  v8::Isolate* isolate = env->isolate();                                      \
  v8::Locker locker(isolate);                                                 \
  v8::HandleScope handle_scope(isolate);                                      \
  v8::Context::Scope context_scope(env->context())

void TimeoutThread::Run() {
  boost::this_thread::sleep(boost::posix_time::milliseconds(delay_));
  func_->Call(args_, env_);
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

void ReadThread::Run() {
  std::string content;
  std::string error;

  try {
    content = file_system_->Read(path_);
  } catch(const std::exception& e) {
    error = e.what();
  } catch(...) {
    error = "Unknown error occurred while reading from " + path_;
  }

  SETUP_THREAD_CONTEXT(env_);

  v8::Local<v8::Object> result = v8::Object::New();
  result->Set(STD_STRING_TO_V8_STRING(isolate, "content"),
              STD_STRING_TO_V8_STRING(isolate, content));
  result->Set(STD_STRING_TO_V8_STRING(isolate, "error"),
              STD_STRING_TO_V8_STRING(isolate, error));
  CallParams params;
  params.push_back(result);
  callback_->Call(params);
  delete this;
}

void WriteThread::Run() {
  std::string error;

  try {
    file_system_->Write(path_, data_);
  } catch(const std::exception& e) {
    error = e.what();
  } catch(...) {
    error = "Unknown error occurred while writing to " + path_;
  }

  SETUP_THREAD_CONTEXT(env_);

  auto result = v8::String::NewFromUtf8(isolate, error.c_str());
  CallParams params;
  params.push_back(result);
  callback_->Call(params);
  delete this;
}

void RemoveThread::Run() {
  std::string error;
  bool removed = true;

  try {
    removed = file_system_->Remove(path_);
  } catch(const boost::filesystem::filesystem_error& e) {
    error = e.what();
  } catch(...) {
    error = "Unknown error occurred while removing " + path_;
  }

  if (!removed && error.length() == 0) {
    error = "Unknown error occurred while removing " + path_;
  }

  SETUP_THREAD_CONTEXT(env_);

  auto result = v8::String::NewFromUtf8(isolate, error.c_str());
  CallParams params;
  params.push_back(result);
  callback_->Call(params);
  delete this;
}

void MoveThread::Run() {
  std::string error;

  try {
    file_system_->Move(from_, to_);
  } catch(const boost::filesystem::filesystem_error& e) {
    error = e.what();
  } catch(...) {
    error = "Unknown error occurred while moving " + from_ + " to " + to_;
  }

  SETUP_THREAD_CONTEXT(env_);

  auto result = v8::String::NewFromUtf8(isolate, error.c_str());
  CallParams params;
  params.push_back(result);
  callback_->Call(params);
  delete this;
}

void StatThread::Run() {
  std::string error;
  FileSystem::StatResult stat_result;

  try {
    stat_result = file_system_->Stat(path_);
  } catch(const boost::filesystem::filesystem_error& e) {
    error = e.what();
  } catch(...) {
    error = "Unknown error occurred while stating " + path_;
  }

  SETUP_THREAD_CONTEXT(env_);

  v8::Local<v8::Object> result = v8::Object::New();
  result->Set(STD_STRING_TO_V8_STRING(isolate, "exists"),
              v8::Boolean::New(stat_result.exists));
  result->Set(STD_STRING_TO_V8_STRING(isolate, "isFile"),
              v8::Boolean::New(stat_result.is_file));
  result->Set(STD_STRING_TO_V8_STRING(isolate, "isDirectory"),
              v8::Boolean::New(stat_result.is_directory));
  result->Set(STD_STRING_TO_V8_STRING(isolate, "lastWriteTime"),
      v8::Number::New(static_cast<double>(stat_result.last_write_time)));
  result->Set(STD_STRING_TO_V8_STRING(isolate, "error"),
              STD_STRING_TO_V8_STRING(isolate, error));
  CallParams params;
  params.push_back(result);
  callback_->Call(params);
  delete this;
}

void ReadCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  if (args.Length() != 2) {
    ADB_THROW_EXCEPTION(isolate, "fileSystem.read requires 2 parameters");
  }
  if (!args[1]->IsFunction()) {
    ADB_THROW_EXCEPTION(isolate,
        "Second argument to fileSystem.read must be a function");
  }

  std::string path = V8_STRING_TO_STD_STRING(args[0]->ToString());
  JsValuePtr callback = JsValuePtr(new JsValue(isolate, args[1]));
  Thread* thread = new ReadThread(isolate, path, callback);
  thread->Start();
}

void WriteCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  if (args.Length() != 3) {
    ADB_THROW_EXCEPTION(isolate, "fileSystem.write requires 3 parameters");
  }
  if (!args[2]->IsFunction()) {
    ADB_THROW_EXCEPTION(isolate,
        "Third argument to fileSystem.write must be a function");
  }

  std::string path = V8_STRING_TO_STD_STRING(args[0]->ToString());
  std::string data = V8_STRING_TO_STD_STRING(args[1]->ToString());
  JsValuePtr callback = JsValuePtr(new JsValue(isolate, args[2]));
  Thread* thread = new WriteThread(isolate, path, data, callback);
  thread->Start();
}

void RemoveCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  if (args.Length() != 2) {
    ADB_THROW_EXCEPTION(isolate, "fileSystem.remove requires 2 parameters");
  }
  if (!args[1]->IsFunction()) {
    ADB_THROW_EXCEPTION(isolate,
        "Second argument to fileSystem.remove must be a function");
  }

  std::string path = V8_STRING_TO_STD_STRING(args[0]->ToString());
  JsValuePtr callback = JsValuePtr(new JsValue(isolate, args[1]));
  Thread* thread = new RemoveThread(isolate, path, callback);
  thread->Start();
}

void MoveCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  if (args.Length() != 3) {
    ADB_THROW_EXCEPTION(isolate, "fileSystem.move requires 3 parameters");
  }
  if (!args[2]->IsFunction()) {
    ADB_THROW_EXCEPTION(isolate,
                        "Third argument to fileSystem.move must be a function");
  }

  std::string from = V8_STRING_TO_STD_STRING(args[0]->ToString());
  std::string to = V8_STRING_TO_STD_STRING(args[1]->ToString());
  JsValuePtr callback = JsValuePtr(new JsValue(isolate, args[2]));
  Thread* thread = new MoveThread(isolate, from, to, callback);
  thread->Start();
}

void StatCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  if (args.Length() != 2) {
    ADB_THROW_EXCEPTION(isolate, "fileSystem.stat requires 2 parameters");
  }
  if (!args[1]->IsFunction()) {
    ADB_THROW_EXCEPTION(isolate,
        "Second argument to fileSystem.stat must be a function");
  }

  std::string path = V8_STRING_TO_STD_STRING(args[0]->ToString());
  JsValuePtr callback = JsValuePtr(new JsValue(isolate, args[1]));
  Thread* thread = new StatThread(isolate, path, callback);
  thread->Start();
}

void ResolveCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  if (args.Length() != 1) {
    ADB_THROW_EXCEPTION(isolate, "fileSystem.resolve requires 1 parameters");
  }

  std::string path = V8_STRING_TO_STD_STRING(args[0]->ToString());
  FileSystemPtr file_system = Environment::GetCurrent(isolate)->GetFileSystem();
  std::string result = file_system->Resolve(path);
  args.GetReturnValue().Set(STD_STRING_TO_V8_STRING(isolate, result));
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

void DoLog(LogSystem::LogLevel level,
           const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::stringstream message;
  for (int idx = 0; idx < args.Length(); ++idx) {
    if (idx > 0) {
      message << " ";
    }
    message << V8_STRING_TO_STD_STRING(args[idx]->ToString());
  }

  std::stringstream source;
  source << "[";
  auto frame = v8::StackTrace::CurrentStackTrace(1)->GetFrame(0);
  source << V8_STRING_TO_STD_STRING(frame->GetScriptName());
  source << ":" << frame->GetLineNumber() << "]";

  auto callback = Environment::GetCurrent(args.GetIsolate())->GetLogSystem();
  (*callback)(level, message.str(), source.str());
}

void LogCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
  DoLog(LogSystem::LOG_LEVEL_LOG, args);
}

void InfoCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
  DoLog(LogSystem::LOG_LEVEL_INFO, args);
}

void WarnCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
  DoLog(LogSystem::LOG_LEVEL_WARN, args);
}

void ErrorCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
  DoLog(LogSystem::LOG_LEVEL_ERROR, args);
}

void TraceCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::stringstream traceback;
  auto frames = v8::StackTrace::CurrentStackTrace(100);
  for (auto idx = 0, limit = frames->GetFrameCount(); idx < limit; ++idx) {
    auto frame = frames->GetFrame(idx);
    traceback << (idx + 1) << ": ";
    std::string name = V8_STRING_TO_STD_STRING(frame->GetFunctionName());
    if (name.size()) {
      traceback << name;
    } else {
      traceback << "/* anonymous */";
    }
    traceback << "() at ";
    traceback << V8_STRING_TO_STD_STRING(frame->GetScriptName());
    traceback << ":" << frame->GetLineNumber() << std::endl;
  }

  auto callback = Environment::GetCurrent(args.GetIsolate())->GetLogSystem();
  (*callback)(LogSystem::LOG_LEVEL_TRACE, traceback.str(), "");
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
