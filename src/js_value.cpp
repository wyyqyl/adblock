#include "js_value.h"
#include "env.h"
#include "js_error.h"

namespace adblock {

JsValue::JsValue(v8::Isolate* isolate, const v8::Handle<v8::Value>& value)
    : value_(isolate, value),
      env_(Environment::GetCurrent(isolate)) {
}

JsValue::JsValue(const v8::Handle<v8::Context>& context,
                 const v8::Handle<v8::Value>& value)
    : value_(context->GetIsolate(), value),
      env_(Environment::GetCurrent(context)) {
}

JsValuePtr JsValue::Call(const JsValueList& args) {
  if (!value_->IsFunction()) {
    throw std::invalid_argument("Attempting to call a non-function");
  }

  CallParam params;
  for (auto it = args.begin(); it != args.end(); ++it) {
    params.push_back((*it)->value_);
  }
  return Call(params);
}

JsValuePtr JsValue::Call(CallParam& args /*= CallParam()*/) {
  if (!value_->IsFunction()) {
    throw std::invalid_argument("Attempting to call a non-function");
  }

  v8::Isolate* isolate = env_->isolate();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Handle<v8::Context> context(env_->context());
  v8::Context::Scope context_scope(context);
  v8::Handle<v8::Object> obj = context->Global();

  v8::TryCatch try_catch;
  auto func = v8::Handle<v8::Function>::Cast<v8::Value>(value_);
  auto result = func->Call(obj, args.size(),
                           args.size() ? &args.front() : nullptr);

  if (try_catch.HasCaught()) {
    throw JsError(isolate, &try_catch);
  }

  return JsValuePtr(new JsValue(isolate, result));
}

}  // namespace adblock
