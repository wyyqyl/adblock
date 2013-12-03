#include "ipc.h"
#include "adblock_impl.h"
#include <sstream>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace adblock {

#define CONNECTING_STATE 0
#define READING_STATE 1
#define WRITING_STATE 2
const LPWSTR kAdblockPipeName = L"\\\\.\\pipe\\adblock";
const LPWSTR kServerPipeName = kAdblockPipeName;
const DWORD kPipeTimeout = 5000;
const DWORD kClientPipeTimeout = 2000;

#define SAFE_CLOSE_HANDLE(handle)                         \
  if (handle != NULL && handle != INVALID_HANDLE_VALUE) { \
    CloseHandle(handle);                                  \
    handle = NULL;                                        \
  }

#define SAFE_SET_EVENT(event)                           \
  if (event != NULL && event != INVALID_HANDLE_VALUE) { \
    SetEvent(event);                                    \
  }

static void PRINT(const WCHAR* fmt, ...) {
  WCHAR buffer[512] = {0};
  va_list args;

  va_start(args, fmt);
  _vsnwprintf(buffer, _countof(buffer), fmt, args);
  va_end(args);

  OutputDebugStringW(buffer);
}

static void PRINT(const CHAR* fmt, ...) {
  CHAR buffer[512] = {0};
  va_list args;

  va_start(args, fmt);
  _vsnprintf(buffer, _countof(buffer), fmt, args);
  va_end(args);

  OutputDebugStringA(buffer);
}

IPCServer::IPCServer() : exit_event_(NULL) {
  adblock_ = nullptr;
  memset(pipe_, 0, sizeof(pipe_));
  memset(ovlp_events_, 0, sizeof(ovlp_events_));
}

IPCServer::~IPCServer() {
  SAFE_CLOSE_HANDLE(exit_event_);
  for (int idx = 0; idx < PIPE_INSTANCES; ++idx) {
    SAFE_CLOSE_HANDLE(ovlp_events_[idx]);
    SAFE_CLOSE_HANDLE(pipe_[idx].pipe);
  }
}

bool IPCServer::Init(AdBlockImpl* adblock) {
  adblock_ = adblock;

  exit_event_ = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (exit_event_ == INVALID_HANDLE_VALUE) {
    PRINT("exit_event_ failed: 0x%08x\n", GetLastError());
    return false;
  }

  for (int idx = 0; idx < PIPE_INSTANCES; ++idx) {
    ovlp_events_[idx] = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (ovlp_events_[idx] == INVALID_HANDLE_VALUE) {
      PRINT("ovlp_.hEvent failed: 0x%08x\n", GetLastError());
      return false;
    }
    pipe_[idx].ovlp.hEvent = ovlp_events_[idx];

    pipe_[idx].pipe = CreateNamedPipeW(
        kAdblockPipeName, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_INSTANCES,
        BUFSIZE, BUFSIZE, kPipeTimeout, NULL);
    if (pipe_[idx].pipe == INVALID_HANDLE_VALUE) {
      PRINT("CreateNamedPipe failed: 0x%08x\n", GetLastError());
      return false;
    }
    ConnectToNewClient(idx);
  }

  return true;
}

void IPCServer::Start() {
  boost::thread t(boost::bind(&IPCServer::Run, this));
  t.detach();
}

void IPCServer::Dispose() {
  SAFE_SET_EVENT(exit_event_);
  for (int idx = 0; idx < PIPE_INSTANCES; ++idx) {
    SAFE_SET_EVENT(ovlp_events_[idx]);
  }
}

void IPCServer::Run() {
  DWORD bytes_ret = 0, index = 0;
  BOOL success = FALSE;
  PPIPE_INST inst = nullptr;

  while (true) {
    index =
        WaitForMultipleObjects(PIPE_INSTANCES, ovlp_events_, FALSE, INFINITE) -
        WAIT_OBJECT_0;
    if (index < 0 || index > PIPE_INSTANCES - 1) {
      return;
    }

    PRINT("Index: %d\n", index);
    if (WaitForSingleObject(exit_event_, 0) == WAIT_OBJECT_0) {
      PRINT("exit_event_\n");
      return;
    }

    inst = &pipe_[index];
    if (inst->pending) {
      // Pending connect operation
      success = GetOverlappedResult(inst->pipe, &inst->ovlp, &bytes_ret, FALSE);
      switch (inst->state) {
        // Pending connect operation
        case CONNECTING_STATE:
          if (!success) {
            PRINT("CONNECTING_STATE failed: 0x%08x\n", GetLastError());
            return;
          }
          inst->state = READING_STATE;
          break;

        // Pending read operation
        case READING_STATE:
          if (!success || bytes_ret == 0) {
            PRINT("READING_STATE failed: 0x%08x\n", GetLastError());
            RestartConnection(index);
            continue;
          }
          inst->bytes_read = bytes_ret;
          inst->state = WRITING_STATE;
          break;

        // Pending write operation
        case WRITING_STATE:
          if (!success || bytes_ret != inst->bytes_to_write) {
            PRINT("WRITING_STATE failed: 0x%08x\n", GetLastError());
            RestartConnection(index);
            continue;
          }
          inst->state = READING_STATE;
          break;

        default:
          PRINT("Invalid pipe state: %d\n", inst->state);
          break;
      }
    }

    switch (inst->state) {
      // READING_STATE:
      // The pipe instance is connected to the client
      // and is ready to read a request from the client.
      case READING_STATE:
        success = ReadFile(inst->pipe, inst->buffer, BUFSIZE, &bytes_ret,
                           &inst->ovlp);

        // The read operation completed successfully.
        if (success && bytes_ret != 0) {
          inst->pending = false;
          inst->state = WRITING_STATE;
          continue;
        }

        // The read operation is still pending.
        if (!success && GetLastError() == ERROR_IO_PENDING) {
          inst->pending = true;
          continue;
        }

        // An error occurred; disconnect from the client.
        RestartConnection(index);
        break;

      // WRITING_STATE:
      // The request was successfully read from the client.
      // Get the reply data and write it to the client.
      case WRITING_STATE:
        ProcessMsg(index);
        if (inst->bytes_to_write == 0) {
          inst->pending = false;
          inst->state = READING_STATE;
          continue;
        }

        success = WriteFile(pipe_, inst->buffer, inst->bytes_to_write,
                            &bytes_ret, &inst->ovlp);
        // The write operation completed successfully.
        if (success && bytes_ret == inst->bytes_to_write) {
          inst->pending = false;
          inst->state = READING_STATE;
          continue;
        }

        // The write operation is still pending.
        if (!success && GetLastError() == ERROR_IO_PENDING) {
          inst->pending = true;
          continue;
        }

        // An error occurred; disconnect from the client.
        RestartConnection(index);
        break;

      default:
        PRINT("Invalid pipe state: %d", inst->state);
        break;
    }
  }
}

void IPCServer::ConnectToNewClient(int idx) {
  PPIPE_INST inst = &pipe_[idx];
  inst->pending = false;
  // Successful asynchronous operation should return 0
  if (ConnectNamedPipe(inst->pipe, &inst->ovlp)) {
    PRINT("ConnectNamedPipe failed: 0x%08x\n", GetLastError());
    return;
  }
  switch (GetLastError()) {
    case ERROR_IO_PENDING:
      inst->pending = true;
      break;
    case ERROR_PIPE_CONNECTED:
      if (SetEvent(ovlp_events_[idx])) {
        break;
      }
    default:
      PRINT("ConnectNamedPipe failed: 0x%08x\n", GetLastError());
      break;
  }
  inst->state = inst->pending ? CONNECTING_STATE : READING_STATE;
}

void IPCServer::RestartConnection(int idx) {
  if (!DisconnectNamedPipe(pipe_[idx].pipe)) {
    PRINT("DisconnectNamedPipe failed: 0x%08x\n", GetLastError());
  }
  ConnectToNewClient(idx);
}

void IPCServer::ProcessMsg(int idx) {
  PPIPE_INST inst = &pipe_[idx];
  std::wstringstream ss;
  ss << inst->buffer;
  try {
    boost::property_tree::wptree root;
    boost::property_tree::read_json(ss, root);
    IPC_CMD cmd = IPC_CMD(root.get<int>(L"cmd", (int)UNKNOWN));
    switch (cmd) {
      case ENABLE_ADBLOCK:
        adblock_->set_enabled(true);
        inst->bytes_to_write = 0;
        break;
      case DISABLE_ADBLOCK:
        adblock_->set_enabled(false);
        inst->bytes_to_write = 0;
        break;
      case ADD_DOMAIN_TO_EXCEPTION_LIST: {
        std::wstring wdomain = root.get(L"domain", L"");
        std::string domain(wdomain.begin(), wdomain.end());
        adblock_->ToggleEnabled(domain, false);
        inst->bytes_to_write = 0;
        break;
      }
      case REMOVE_DOMAIN_FROM_EXCEPTION_LIST: {
        std::wstring wdomain = root.get(L"domain", L"");
        std::string domain(wdomain.begin(), wdomain.end());
        adblock_->ToggleEnabled(domain, true);
        inst->bytes_to_write = 0;
        break;
      }
      default:
        inst->bytes_to_write = 0;
        break;
    }
  }
  catch (const boost::property_tree::json_parser_error& e) {
    PRINT("buffer_: %s", inst->buffer);
    PRINT("json_parser_error: %s", e.what());
  }
  catch (const boost::property_tree::ptree_bad_path& e) {
    PRINT("ptree_bad_path: %s", e.what());
  }
}

IPCClient::IPCClient() : pipe_(NULL) {}

IPCClient::~IPCClient() { SAFE_CLOSE_HANDLE(pipe_); }

bool IPCClient::Init() {
  while (true) {
    pipe_ = CreateFileW(kServerPipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        OPEN_EXISTING, 0, NULL);
    if (pipe_ != INVALID_HANDLE_VALUE) {
      break;
    }

    // Exit if an error other than ERROR_PIPE_BUSY occurs.
    if (GetLastError() != ERROR_PIPE_BUSY) {
      PRINT("Could not open pipe: 0x%08x\n", GetLastError());
      return false;
    }

    // All pipe instances are busy, so wait for 2 seconds.
    if (!WaitNamedPipeW(kServerPipeName, kClientPipeTimeout)) {
      PRINT("Could not open pipe: %d seconds wait timed out",
            kClientPipeTimeout);
      return false;
    }
  }
  return true;
}

void IPCClient::Send(const std::wstring& msg) {
  DWORD bytes_written = 0;
  if (!WriteFile(pipe_, msg.c_str(), sizeof(WCHAR) * (msg.size() + 1),
                 &bytes_written, NULL)) {
    PRINT("Write data to pipe failed: 0x%08x\n", GetLastError());
  }
}

}  // namespace adblock
