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

JsValuePtr JsValue::Call(const JsValueList& args /*= JsValueList()*/,
                         JsValuePtr recv /*= JsValuePtr()*/) {
  if (!value_->IsFunction()) {
    throw std::runtime_error("Attempting to call a non-function");
  }

  v8::Isolate* isolate = env_->isolate();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Handle<v8::Context> context = env_->context();
  v8::Context::Scope context_scope(context);

  v8::Handle<v8::Object> obj;
  if (!recv) {
    obj = context->Global();
  } else if (!recv->IsObject()) {
    throw std::runtime_error("`this` pointer has to be a object");
  } else {
    obj = v8::Handle<v8::Object>::Cast<v8::Value>(recv->value_);
  }

  v8::TryCatch try_catch;
  v8::Handle<v8::Value> *argv = nullptr;
  int argc = args.size();
  if (argc) {
    argv = new v8::Handle<v8::Value>[args.size()];
    for (size_t idx = 0; idx < args.size(); ++idx) {
      argv[idx] = args[idx]->value_;
    }
  }
  auto func = v8::Handle<v8::Function>::Cast<v8::Value>(value_);
  auto result = func->Call(obj, argc, argv);
  if (argv) {
    delete[] argv;
    argv = nullptr;
  }

  if (try_catch.HasCaught()) {
    throw JsError(isolate, &try_catch);
  }

  return JsValuePtr(new JsValue(isolate, result));
}

}  // namespace adblock
