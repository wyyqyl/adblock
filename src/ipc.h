#ifndef IPC_H_
#define IPC_H_

#include <Windows.h>
#include <string>

namespace adblock {

#define BUFLENGTH 512
#define BUFSIZE BUFLENGTH * sizeof(WCHAR)
#define PIPE_INSTANCES 4

typedef struct _PIPE_INSTANCE {
  bool pending;
  HANDLE pipe;
  DWORD state;
  DWORD bytes_read;
  DWORD bytes_to_write;
  OVERLAPPED ovlp;
  WCHAR buffer[BUFLENGTH];
} PIPE_INST, *PPIPE_INST;

class AdBlockImpl;
class IPCServer {
 public:
  IPCServer();
  ~IPCServer();
  bool Init(AdBlockImpl* adblock);
  void Start();
  void Dispose();

 private:
  PIPE_INST pipe_[PIPE_INSTANCES];
  HANDLE ovlp_events_[PIPE_INSTANCES];
  HANDLE exit_event_;
  AdBlockImpl* adblock_;

  void Run();
  void ConnectToNewClient(int idx);
  void RestartConnection(int idx);
  void ProcessMsg(int idx);
};

class IPCClient {
 public:
  IPCClient();
  ~IPCClient();
  bool Init();
  void Send(const std::wstring& msg);

 private:
  HANDLE pipe_;
};

}  // namespace adblock

#endif  // IPC_H_
