#ifndef ENV_H_
#define ENV_H_

#include "js_value.h"
#include "file_system.h"
#include "log_system.h"
#include "web_request.h"

#include <boost/unordered/unordered_map.hpp>
#include <boost/function.hpp>

namespace adblock {

// Pick an index that's hopefully out of the way when we're embedded inside
// another application. Performance-wise or memory-wise it doesn't matter:
// Context::SetAlignedPointerInEmbedderData() is backed by a FixedArray,
// worst case we pay a one-time penalty for resizing the array.
#ifndef ADB_CONTEXT_EMBEDDER_DATA_INDEX
#define ADB_CONTEXT_EMBEDDER_DATA_INDEX 32
#endif

typedef boost::function<void(const JsValueList& args)> EventCallback;
typedef boost::unordered_map<std::string, EventCallback> EventMap;

class Environment {
 public:
  static Environment* New(const v8::Local<v8::Context>& context);
  static Environment* GetCurrent(v8::Isolate* isolate);
  static Environment* GetCurrent(const v8::Local<v8::Context>& context);

  void SetEventCallback(const std::string& name, EventCallback callback);
  void RemoveEventCallback(const std::string& name);
  void TriggerEvent(const std::string& name, const JsValueList& args);

  v8::Isolate* isolate() const;

  v8::Local<v8::Value> Evaluate(const std::string& source,
                                const std::string& file_name);

  void Dispose();

  v8::Local<v8::Context> context() const;

  void SetFileSystem(FileSystemPtr file_system);
  FileSystemPtr GetFileSystem();

  void SetLogSystem(LogSystemPtr log_system);
  LogSystemPtr GetLogSystem();

  void SetWebRequest(WebRequestPtr web_reqeust);
  WebRequestPtr GetWebRequest();

 private:
  explicit Environment(const v8::Local<v8::Context>& context);
  ~Environment();

  enum ContextEmbedderDataIndex {
    kContextEmbedderDataIndex = ADB_CONTEXT_EMBEDDER_DATA_INDEX
  };

  v8::Isolate* const isolate_;
  JsData<v8::Context> context_;
  EventMap event_map_;
  FileSystemPtr file_system_;
  LogSystemPtr log_system_;
  WebRequestPtr web_request_;
};

}  // namespace adblock

#endif  // ENV_H_
