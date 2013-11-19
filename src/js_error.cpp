#include "js_error.h"

#include <v8/v8.h>
#include <sstream>
#include <string>

namespace {

std::string ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch) {
  v8::HandleScope handle_scope(isolate);
  std::stringstream error;

  v8::String::Utf8Value exception(try_catch->Exception());
  v8::Handle<v8::Message> message = try_catch->Message();

  if (message.IsEmpty()) {
    // V8 didn't provide any extra information about this error; just
    // print the exception.
    return std::string(*exception);
  }

  // Print (filename):(line number): (message).
  v8::String::Utf8Value filename(message->GetScriptResourceName());
  error << *filename << ":" << message->GetLineNumber() << ": " << *exception
        << "\n";

  // Print line of source code.
  v8::String::Utf8Value sourceline(message->GetSourceLine());
  error << *sourceline << "\n";

  // Print wavy underline (GetUnderline is deprecated).
  int start = message->GetStartColumn();
  for (int i = 0; i < start; i++) {
    error << " ";
  }
  int end = message->GetEndColumn();
  for (int i = start; i < end; i++) {
    error << "^";
  }
  error << "\n";

  v8::String::Utf8Value stack_trace(try_catch->StackTrace());
  if (stack_trace.length() > 0) {
    error << *stack_trace;
  }

  return error.str();
}

}  // namespace

namespace adblock {

JsError::JsError(v8::Isolate* isolate, v8::TryCatch* try_catch)
    : std::runtime_error(ReportException(isolate, try_catch)) {}

}  // namespace adblock
