#include "adblock_impl.h"
#include "env.h"
#include "js_object.h"

#include <boost/bind.hpp>

namespace adblock {

extern std::string js_sources[];

void CreateInstance(AdBlockPtr *adblock) {
  *adblock = AdBlockPtr(new AdBlockImpl());
}

AdBlockImpl::AdBlockImpl() : is_first_run_(false) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = v8::Context::New(isolate);
  v8::Context::Scope context_scope(context);

  env_ = Environment::New(context);
  js_object::Setup(env_);

  env_->SetEventCallback("init", boost::bind(&AdBlockImpl::InitDone, this, _1));
  for (int idx = 0; !js_sources[idx].empty(); idx += 2) {
    env_->Evaluate(js_sources[idx + 1], js_sources[idx]);
  }

  // Wait for signal from AdBlockImpl::InitDone()
}

AdBlockImpl::~AdBlockImpl() {
  if (env_ != nullptr) {
    v8::Locker locker(env_->isolate());
    v8::HandleScope handle_scope(env_->isolate());
    env_->Dispose();
    env_ = nullptr;
  }
}

void AdBlockImpl::InitDone(const JsValueList& args) {
  env_->RemoveEventCallback("init");
  is_first_run_ = args.size() && args.front()->BooleanValue();
  // Emit signal
}

}  // namespace adblock
