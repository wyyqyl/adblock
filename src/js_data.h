#ifndef JS_DATA_H_
#define JS_DATA_H_

#include <v8/v8.h>

namespace adblock {

template <class T>
inline v8::Local<T> PersistentToLocal(v8::Isolate* isolate,
                                      const v8::Persistent<T>& persistent) {
  if (persistent.IsWeak()) {
    return WeakPersistentToLocal(isolate, persistent);
  }
  return StrongPersistentToLocal(persistent);
}

template <class T>
inline v8::Local<T> StrongPersistentToLocal(
    const v8::Persistent<T>& persistent) {
  return *reinterpret_cast<v8::Local<T>*>(
              const_cast<v8::Persistent<T>*>(&persistent));
}

template <class T>
inline v8::Local<T> WeakPersistentToLocal(v8::Isolate* isolate,
                                          const v8::Persistent<T>& persistent) {
  return v8::Local<T>::New(isolate, persistent);
}

template <typename T>
class JsData {
 public:
  JsData() { Reset(nullptr, v8::Persistent<T>()); }

  JsData(v8::Isolate* isolate, const v8::Persistent<T>& data) {
    Reset(isolate, data);
  }

  JsData(v8::Isolate* isolate, const v8::Handle<T>& data) {
    Reset(isolate, data);
  }

  ~JsData() { data_.Reset(); }

  void Reset(v8::Isolate* isolate, const v8::Persistent<T>& data) {
    data_.Reset(isolate, data);
  }

  void Reset(v8::Isolate* isolate, const v8::Handle<T>& data) {
    data_.Reset(isolate, data);
  }

  operator v8::Local<T>() const { return StrongPersistentToLocal(data_); }

  T* operator->() const { return *operator v8::Local<T>(); }

  operator v8::Persistent<T>() const { return data_; }

 private:
  v8::Persistent<T> data_;
};

}  // namespace adblock

#endif  // JS_DATA_H_
