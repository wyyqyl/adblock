#include "adblock_impl.h"
#include "js_object.h"
#include <ctime>

#ifdef WIN32
#include <Windows.h>
#endif  // WIN32

#ifdef ENABLE_DEBUGGER_SUPPORT
#include <v8/v8-debug.h>
#endif  // ENABLE_DEBUGGER_SUPPORT

#include <boost/bind.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/filesystem/operations.hpp>

#include <glog/logging.h>

namespace adblock {

extern std::string js_sources[];

#ifdef ENABLE_DEBUGGER_SUPPORT
v8::Persistent<v8::Context> debug_message_context;

void DispatchDebugMessages() {
  // We are in some random thread. We should already have v8::Locker acquired
  // (we requested this when registered this callback). We was called
  // because new debug messages arrived; they may have already been processed,
  // but we shouldn't worry about this.
  //
  // All we have to do is to set context and call ProcessDebugMessages.
  //
  // We should decide which V8 context to use here. This is important for
  // "evaluate" command, because it must be executed some context.
  // In our sample we have only one context, so there is nothing really to
  // think about.
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      v8::Local<v8::Context>::New(isolate, debug_message_context);
  v8::Context::Scope scope(context);

  v8::Debug::ProcessDebugMessages();
}
#endif  // ENABLE_DEBUGGER_SUPPORT

void CreateInstance(AdBlockPtr* adblock) {
  *adblock = nullptr;
  AdBlockImpl* result = new AdBlockImpl();
  if (result->Init()) {
    *adblock = AdBlockPtr(result);
  } else {
#ifdef WIN32
    OutputDebugStringA("Failed to initialize adblock!!!");
#endif  // WIN32
    delete result;
  }
}

AdBlockImpl::AdBlockImpl() : downloading_count_(0) {}

AdBlockImpl::~AdBlockImpl() {
  if (env_ != nullptr) {
    v8::Locker locker(env_->isolate());
    v8::HandleScope handle_scope(env_->isolate());
    env_->Dispose();
    env_ = nullptr;
  }
}

bool AdBlockImpl::Init() {
  try {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = v8::Context::New(isolate);
    v8::Context::Scope context_scope(context);

    env_ = Environment::New(context);
    js_object::Setup(env_);

    env_->SetEventCallback("downloadStart",
                           boost::bind(&AdBlockImpl::DownloadStart, this, _1));
    env_->SetEventCallback(
        "downloadFinished",
        boost::bind(&AdBlockImpl::DownloadFinished, this, _1));

#ifdef ENABLE_DEBUGGER_SUPPORT
    debug_message_context.Reset(isolate, context);
    v8::Debug::SetDebugMessageDispatchHandler(DispatchDebugMessages, true);
    v8::Debug::EnableAgent("adblock", 9222, false);
    getchar();
#endif  // ENABLE_DEBUGGER_SUPPORT

    for (int idx = 0; !js_sources[idx].empty(); idx += 2) {
      env_->Evaluate(js_sources[idx + 1], js_sources[idx]);
    }

    auto fun_name = v8::String::NewFromUtf8(isolate, "initAdblock");
    auto process_val = context->Global()->Get(fun_name);
    JsValue(isolate, process_val).Call();
  }
  catch (const std::exception& e) {
#ifdef WIN32
    OutputDebugStringA(e.what());
#endif  // WIN32
    std::cerr << e.what() << std::endl;
    return false;
  }
  return true;
}

std::string AdBlockImpl::CheckFilterMatch(const std::string& location,
                                          const std::string& type,
                                          const std::string& document) {
  SETUP_THREAD_CONTEXT(env_);

  JsValue func(isolate, env_->Evaluate("API.checkFilterMatch"));
  CallParams params;
  params.emplace_back(v8::String::NewFromUtf8(isolate, location.c_str()));
  params.emplace_back(v8::String::NewFromUtf8(isolate, type.c_str()));
  params.emplace_back(v8::String::NewFromUtf8(isolate, document.c_str()));

  return V8_STRING_TO_STD_STRING(
      v8::Local<v8::String>::Cast<v8::Value>(func.Call(params)));
}

std::string AdBlockImpl::GetElementHidingSelectors(const std::string& domain) {
  SETUP_THREAD_CONTEXT(env_);

  JsValue func(isolate, env_->Evaluate("API.getElementHidingSelectors"));
  CallParams params;
  params.emplace_back(v8::String::NewFromUtf8(isolate, domain.c_str()));

  return V8_STRING_TO_STD_STRING(
      v8::Local<v8::String>::Cast<v8::Value>(func.Call(params)));
}

bool AdBlockImpl::IsWhitelisted(const std::string& url,
                                const std::string& parent_url,
                                const std::string& type) {
  SETUP_THREAD_CONTEXT(env_);

  JsValue func(isolate, env_->Evaluate("API.isWhitelisted"));
  CallParams params;
  params.emplace_back(v8::String::NewFromUtf8(isolate, url.c_str()));
  if (parent_url.length()) {
    params.emplace_back(v8::String::NewFromUtf8(isolate, parent_url.c_str()));
    if (type.length()) {
      params.emplace_back(v8::String::NewFromUtf8(isolate, type.c_str()));
    }
  }

  return func.Call(params)->BooleanValue();
}

void AdBlockImpl::ToggleEnabled(const std::string& url, bool enabled) {
  SETUP_THREAD_CONTEXT(env_);

  JsValue func(isolate, env_->Evaluate("API.toggleEnabled"));
  CallParams params;
  params.emplace_back(v8::String::NewFromUtf8(isolate, url.c_str()));
  params.emplace_back(v8::Boolean::New(enabled));
  func.Call(params);
}

std::string AdBlockImpl::GenerateCSSContent() {
  SETUP_THREAD_CONTEXT(env_);

  JsValue func(isolate, env_->Evaluate("API.generateCSSContent"));
  return JsValue(isolate, func.Call()).ToStdString();
}

void AdBlockImpl::Report(const std::string& type,
                         const std::string& documentUrl, const std::string& url,
                         const std::string& rule) {
  boost::property_tree::ptree root;
  std::stringstream ss;

  root.put<std::time_t>("time", std::time(nullptr));
  root.put<std::string>("type", type);
  root.put<int>("pid", GetCurrentProcessId());
  root.put<std::string>("process", GetCurrentProcessName());
  root.put<std::string>("website", documentUrl);
  if (url.length()) {
    root.put<std::string>("location", url);
    if (rule.length()) {
      root.put<std::string>("rule", rule);
    }
  }

  boost::property_tree::write_json(ss, root, false);
  sender_.Send(ss.str());
}

bool AdBlockImpl::block_ads() { return config_.block_ads(); }

bool AdBlockImpl::block_malware() { return config_.block_malware(); }

bool AdBlockImpl::dont_track_me() { return config_.dont_track_me(); }

std::string AdBlockImpl::GetCurrentProcessName() {
  char file_name[MAX_PATH] = {0};
  if (GetModuleFileNameA(NULL, file_name, MAX_PATH)) {
    boost::filesystem::path path(file_name);
    return path.filename().string();
  }
  return "";
}

std::uint8_t AdBlockImpl::GetDownloadingTask() { return downloading_count_; }

void AdBlockImpl::DownloadStart(const JsValueList& args) {
  ++downloading_count_;
}

void AdBlockImpl::DownloadFinished(const JsValueList& args) {
  --downloading_count_;
}

}  // namespace adblock
