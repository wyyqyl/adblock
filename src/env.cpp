#include "env.h"
#include "js_error.h"

namespace {

v8::Handle<v8::Script> CompileScript(v8::Isolate* isolate,
                                     const std::string& source,
                                     const std::string& file_name) {
  auto v8_source = v8::String::NewFromUtf8(isolate, source.c_str());
  if (file_name.length()) {
    auto v8_file_name = v8::String::NewFromUtf8(isolate, file_name.c_str());
    return v8::Script::Compile(v8_source, v8_file_name);
  }
  return v8::Script::Compile(v8_source);
}

}  // namespace

namespace adblock {

Environment::Environment(const v8::Local<v8::Context>& context)
    : isolate_(context->GetIsolate()),
      context_(context->GetIsolate(), context) {
  event_map_.clear();
  timeout_threads_.clear();
}

Environment::~Environment() {
  context_->SetAlignedPointerInEmbedderData(kContextEmbedderDataIndex,
                                            nullptr);
  for (auto it = timeout_threads_.begin(); it != timeout_threads_.end(); ++it) {
    (*it)->interrupt();
    (*it)->join();
    delete *it;
    *it = nullptr;
  }
}

Environment* Environment::New(const v8::Local<v8::Context>& context) {
  Environment* env = new Environment(context);
  context->SetAlignedPointerInEmbedderData(kContextEmbedderDataIndex, env);
  return env;
}

Environment* Environment::GetCurrent(v8::Isolate* isolate) {
  return GetCurrent(isolate->GetCurrentContext());
}

Environment* Environment::GetCurrent(const v8::Local<v8::Context>& context) {
  return static_cast<Environment*>(
      context->GetAlignedPointerFromEmbedderData(kContextEmbedderDataIndex));
}

void Environment::SetEventCallback(const std::string& name,
                                   EventCallback callback) {
  event_map_[name] = callback;
}

void Environment::RemoveEventCallback(const std::string& name) {
  event_map_.erase(name);
}

void Environment::TriggerEvent(const std::string& name,
                               const JsValueList& args) {
  auto it = event_map_.find(name);
  if (it != event_map_.end()) {
    it->second(args);
  }
}

v8::Isolate* Environment::isolate() const {
  return isolate_;
}

v8::Local<v8::Context> Environment::context() const {
  return context_;
}

v8::Local<v8::Value> Environment::Evaluate(const std::string& source,
                                           const std::string& file_name) {
  v8::EscapableHandleScope handle_scope(isolate_);
  v8::TryCatch try_catch;
  auto script = CompileScript(isolate_, source, file_name);
  if (try_catch.HasCaught()) {
    throw JsError(isolate_, &try_catch);
  }
  auto result = script->Run();
  if (try_catch.HasCaught()) {
    throw JsError(isolate_, &try_catch);
  }
  return handle_scope.Escape(result);
}

void Environment::Dispose() {
  delete this;
}

void Environment::SetFileSystem(FileSystemPtr file_system) {
  if (!file_system) {
    throw std::invalid_argument("FileSystem can't be null");
  }
  file_system_ = file_system;
}

FileSystemPtr Environment::GetFileSystem() {
  if (!file_system_) {
    file_system_.reset(new DefaultFileSystem());
  }
  return file_system_;
}

void Environment::SetLogSystem(LogSystemPtr log_system) {
  if (!log_system) {
    throw std::invalid_argument("LogSystem can't be null");
  }
  log_system_ = log_system;
}

adblock::LogSystemPtr Environment::GetLogSystem() {
  if (!log_system_) {
    log_system_.reset(new DefaultLogSystem());
  }
  return log_system_;
}

void Environment::SetWebRequest(WebRequestPtr web_reqeust) {
  if (!web_reqeust) {
    throw std::invalid_argument("WebRequest can't be null");
  }
  web_request_ = web_reqeust;
}

adblock::WebRequestPtr Environment::GetWebRequest() {
  if (!web_request_) {
    web_request_.reset(new DefaultWebRequest());
  }
  return web_request_;
}

}  // namespace adblock
