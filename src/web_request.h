#ifndef WEB_REQUEST_H_
#define WEB_REQUEST_H_

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>

namespace adblock {

class WebRequest {
 public:
  typedef std::vector<std::pair<std::string, std::string>> HeaderList;
  struct ServerResponse {
    int status;
    int response_status;
    HeaderList response_headers;
    std::string response_text;
  };
  enum NetworkStatus {
    NS_OK = 0,
    NS_ERROR_FAILURE = 0x80004005,
    NS_ERROR_OUT_OF_MEMORY = 0x8007000e,
    NS_ERROR_MALFORMED_URI = 0x804b000a,
    NS_ERROR_CONNECTION_REFUSED = 0x804b000d,
    NS_ERROR_NET_TIMEOUT = 0x804b000e,
    NS_ERROR_NO_CONTENT = 0x804b0011,
    NS_ERROR_UNKNOWN_PROTOCOL = 0x804b0012,
    NS_ERROR_NET_RESET = 0x804b0014,
    NS_ERROR_UNKNOWN_HOST = 0x804b001e,
    NS_ERROR_REDIRECT_LOOP = 0x804b001f,
    NS_ERROR_UNKNOWN_PROXY_HOST = 0x804b002a,
    NS_ERROR_NET_INTERRUPT = 0x804b0047,
    NS_ERROR_UNKNOWN_PROXY_CONNECTION_REFUSED = 0x804b0048,
    NS_CUSTOM_ERROR_BASE = 0x80850000,
    NS_ERROR_NOT_INITIALIZED = 0xc1f30001
  };

  virtual ~WebRequest() {}
  virtual ServerResponse Get(const std::string& url,
                             const HeaderList& headers) const = 0;
};

typedef boost::shared_ptr<WebRequest> WebRequestPtr;

class DefaultWebRequest : public WebRequest {
 public:
  WebRequest::ServerResponse Get(const std::string& url,
                                 const HeaderList& headers) const;
};

}  // namespace adblock

#endif  // WEB_REQUEST_H_
