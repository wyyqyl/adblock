#include "js_value.h"
#include "env.h"
#include "js_error.h"

namespace adblock {

JsValue::JsValue(v8::Isolate* isolate, const v8::Handle<v8::Value>& value)
    : value_(isolate, value) {
}

v8::Local<v8::Value> JsValue::Call(const JsValueList& args, Environment* env) {
  if (!value_->IsFunction()) {
    throw std::invalid_argument("Attempting to call a non-function");
  }

  v8::Isolate* isolate = env->isolate();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(env->context());

  CallParams params;
  for (auto it = args.begin(); it != args.end(); ++it) {
    params.push_back((*it)->value_);
  }
  return Call(params);
}

v8::Local<v8::Value> JsValue::Call(const CallParams& args /*= CallParams()*/,
                         Environment* env /*= nullptr*/) {
  if (!value_->IsFunction()) {
    throw std::invalid_argument("Attempting to call a non-function");
  }

  v8::Isolate* isolate;
  if (env) {
    isolate = env->isolate();
  } else {
    isolate = v8::Isolate::GetCurrent();
  }
  v8::Local<v8::Context> context(isolate->GetCurrentContext());
  v8::Handle<v8::Object> obj = context->Global();
  v8::EscapableHandleScope handle_scope(isolate);

  v8::TryCatch try_catch;
  auto func = v8::Handle<v8::Function>::Cast<v8::Value>(value_);
  auto result = func->Call(obj, args.size(), args.size() ?
      const_cast<v8::Handle<v8::Value>*>(&args.front()) : nullptr);

  if (try_catch.HasCaught()) {
    throw JsError(isolate, &try_catch);
  }

  return handle_scope.Escape(result);
}

}  // namespace adblock
