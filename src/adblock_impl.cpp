#include "adblock_impl.h"
#include "js_object.h"

#define ENABLE_DEBUGGER_SUPPORT

#ifdef ENABLE_DEBUGGER_SUPPORT
#include <v8/v8-debug.h>
#endif  // ENABLE_DEBUGGER_SUPPORT

#include <boost/bind.hpp>

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

void CreateInstance(AdBlockPtr *adblock) {
  *adblock = nullptr;
  AdBlockImpl *result = new AdBlockImpl();
  if (result->Init()) {
    *adblock = AdBlockPtr(result);
  } else {
    delete result;
  }
}

AdBlockImpl::AdBlockImpl()
    : is_first_run_(false),
      initialized_(false),
      collapse_(true) {
}

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

    env_->SetEventCallback("init",
                           boost::bind(&AdBlockImpl::InitDone, this, _1));
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
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return false;
  }

  while (!initialized_) {
    boost::this_thread::sleep(boost::posix_time::microseconds(10));
  }
  return true;
}

void AdBlockImpl::InitDone(const JsValueList& args) {
  env_->RemoveEventCallback("init");
  is_first_run_ = args.size() && args.front()->BooleanValue();
  initialized_ = true;
}

FilterPtr AdBlockImpl::CheckFilterMatch(const std::string& location,
                                        const std::string& type,
                                        const std::string& document) {
  v8::Isolate* isolate = env_->isolate();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(env_->context());

  JsValue func(isolate, env_->Evaluate("API.checkFilterMatch"));
  CallParams params;
  params.emplace_back(v8::String::NewFromUtf8(isolate, location.c_str()));
  params.emplace_back(v8::String::NewFromUtf8(isolate, type.c_str()));
  params.emplace_back(v8::String::NewFromUtf8(isolate, document.c_str()));

  auto result = v8::Local<v8::Object>::Cast<v8::Value>(func.Call(params));
  if (result->IsNull()) {
    return FilterPtr(new Filter());
  }

  std::string name = V8_STRING_TO_STD_STRING(result->GetConstructorName());
  if (name == "BlockingFilter") {
    auto collapse = result->Get(v8::String::NewFromUtf8(isolate, "collapse"));
    return FilterPtr(new Filter(Filter::TYPE_BLOCKING,
        collapse->IsNull() ? collapse_ : collapse->BooleanValue()));
  }

  Filter::Type filter_type = Filter::TYPE_INVALID;
  if (name == "WhitelistFilter") {
    filter_type = Filter::TYPE_EXCEPTION;
  } else if (name == "ElemHideFilter") {
    filter_type = Filter::TYPE_ELEMHIDE;
  } else if (name == "ElemHideException") {
    filter_type = Filter::TYPE_ELEMHIDE_EXCEPTION;
  } else if (name == "CommentFilter") {
    filter_type = Filter::TYPE_COMMENT;
  }
  return FilterPtr(new Filter(filter_type));
}

}  // namespace adblock
