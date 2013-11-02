#include "web_request.h"
#include <curl/curl.h>
#include <sstream>

namespace {

struct HeaderData {
  HeaderData() {
    status = 0;
    expected_status = true;
  }

  int status;
  bool expected_status;
  std::vector<std::string> headers;
};

unsigned int ConvertErrorCode(CURLcode code) {
  switch (code) {
    case CURLE_OK:
      return adblock::WebRequest::NS_OK;
    case CURLE_FAILED_INIT:
      return adblock::WebRequest::NS_ERROR_NOT_INITIALIZED;
    case CURLE_UNSUPPORTED_PROTOCOL:
      return adblock::WebRequest::NS_ERROR_UNKNOWN_PROTOCOL;
    case CURLE_URL_MALFORMAT:
      return adblock::WebRequest::NS_ERROR_MALFORMED_URI;
    case CURLE_COULDNT_RESOLVE_PROXY:
      return adblock::WebRequest::NS_ERROR_UNKNOWN_PROXY_HOST;
    case CURLE_COULDNT_RESOLVE_HOST:
      return adblock::WebRequest::NS_ERROR_UNKNOWN_HOST;
    case CURLE_COULDNT_CONNECT:
      return adblock::WebRequest::NS_ERROR_CONNECTION_REFUSED;
    case CURLE_OUT_OF_MEMORY:
      return adblock::WebRequest::NS_ERROR_OUT_OF_MEMORY;
    case CURLE_OPERATION_TIMEDOUT:
      return adblock::WebRequest::NS_ERROR_NET_TIMEOUT;
    case CURLE_TOO_MANY_REDIRECTS:
      return adblock::WebRequest::NS_ERROR_REDIRECT_LOOP;
    case CURLE_GOT_NOTHING:
      return adblock::WebRequest::NS_ERROR_NO_CONTENT;
    case CURLE_SEND_ERROR:
      return adblock::WebRequest::NS_ERROR_NET_RESET;
    case CURLE_RECV_ERROR:
      return adblock::WebRequest::NS_ERROR_NET_RESET;
    default:
      return adblock::WebRequest::NS_CUSTOM_ERROR_BASE + code;
  }
}

size_t ReceiveData(char* ptr, size_t size, size_t nmemb, void* userdata) {
  std::stringstream* stream = static_cast<std::stringstream*>(userdata);
  stream->write(ptr, size * nmemb);
  return nmemb;
}

size_t ReceiveHeader(char* ptr, size_t size, size_t nmemb, void* userdata) {
  HeaderData* data = static_cast<HeaderData*>(userdata);
  std::string header(ptr, size * nmemb);
  if (data->expected_status) {
    // Parse the status code out of something like "HTTP/1.1 200 OK"
    const std::string prefix("HTTP/1.");
    size_t prefix_len = prefix.length();
    if (header.length() >= prefix_len + 2 &&
        !header.compare(0, prefix_len, prefix) &&
        isdigit(header[prefix_len]) && isspace(header[prefix_len + 1])) {
      size_t status_start = prefix_len + 2;
      while (status_start < header.length() &&
             isspace(header[status_start])) {
        ++status_start;
      }

      size_t status_end = status_start;
      while (status_end < header.length() && isdigit(header[status_end])) {
        ++status_end;
      }

      if (status_end > status_start && status_end < header.length() &&
          isspace(header[status_end])) {
        std::istringstream(header.substr(
          status_start, status_end - status_start)) >> data->status;
        data->headers.clear();
        data->expected_status = false;
      }
    }
  } else {
    size_t header_end = header.length();
    while (header_end > 0 && isspace(header[header_end - 1])) {
      --header_end;
    }

    if (header_end) {
      data->headers.push_back(header.substr(0, header_end));
    } else {
      data->expected_status = true;
    }
  }
  return nmemb;
}

}  // namespace

namespace adblock {

WebRequest::ServerResponse DefaultWebRequest::Get(
    const std::string& url,
    const HeaderList& headers) const {
  ServerResponse result;
  result.status = NS_ERROR_NOT_INITIALIZED;
  result.response_status = 0;

  CURL* curl = curl_easy_init();
  if (!curl) {
    return result;
  }

  std::stringstream response_text;
  HeaderData header_data;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ReceiveData);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_text);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, ReceiveHeader);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_data);

  struct curl_slist* header_list = nullptr;
  for (auto it = headers.begin(); it != headers.end(); ++it) {
    header_list = curl_slist_append(header_list,
                                    (it->first + ": " + it->second).c_str());
  }
  if (header_list) {
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
  }

  result.status = ConvertErrorCode(curl_easy_perform(curl));
  result.response_status = header_data.status;
  result.response_text = response_text.str();
  for (auto it = header_data.headers.begin();
       it != header_data.headers.end(); ++it) {
    // Parse header name and value out of something like "Foo: bar"
    std::string header = *it;
    size_t colon_pos = header.find(':');
    if (colon_pos != std::string::npos) {
      size_t name_start = 0;
      size_t name_end = colon_pos;
      while (name_end > name_start && isspace(header[name_end - 1])) {
        --name_end;
      }

      size_t value_start = colon_pos + 1;
      while (value_start < header.length() && isspace(header[value_start])) {
        ++value_start;
      }

      size_t value_end = header.length();
      if (name_end > name_start && value_end > value_start) {
        std::string name = header.substr(name_start, name_end - name_start);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        std::string value = header.substr(value_start, value_end - value_start);
        result.response_headers.push_back(
            std::pair<std::string, std::string>(name, value));
      }
    }
  }

  if (header_list) {
    curl_slist_free_all(header_list);
  }
  curl_easy_cleanup(curl);
  return result;
}

}  // namespace adblock
