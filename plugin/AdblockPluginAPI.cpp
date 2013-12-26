#include "JSObject.h"
#include "variant_list.h"
#include "DOM/Document.h"
#include "global/config.h"
#include "AdblockPluginAPI.h"

#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "Ws2_32.lib")
#ifdef _DEBUG
#pragma comment(lib, "v8_snapshot-sd.lib")
#pragma comment(lib, "v8_base.ia32-sd.lib")
#pragma comment(lib, "icuuc-sd.lib")
#pragma comment(lib, "icui18n-sd.lib")
#pragma comment(lib, "libcurl-sd.lib")
#pragma comment(lib, "libeay32-sd.lib")
#pragma comment(lib, "ssleay32-sd.lib")
#pragma comment(lib, "zlib-sd.lib")
#pragma comment(lib, "libglog-sd.lib")
#else
#pragma comment(lib, "v8_snapshot-s.lib")
#pragma comment(lib, "v8_base.ia32-s.lib")
#pragma comment(lib, "icuuc-s.lib")
#pragma comment(lib, "icui18n-s.lib")
#pragma comment(lib, "libcurl-s.lib")
#pragma comment(lib, "libeay32-s.lib")
#pragma comment(lib, "ssleay32-s.lib")
#pragma comment(lib, "zlib-s.lib")
#pragma comment(lib, "libglog-s.lib")
#endif

std::string AdblockPluginAPI::CheckFilterMatch(const std::string& location,
                                               const std::string& type,
                                               const std::string& document) {
  return adblock_->CheckFilterMatch(location, type, document);
}

std::string AdblockPluginAPI::GetElementHidingSelectors(
    const std::string& domain) {
  return adblock_->GetElementHidingSelectors(domain);
}

bool AdblockPluginAPI::IsWhitelisted(const std::string& url,
                                     const std::string& parent_url,
                                     const std::string& type) {
  return adblock_->IsWhitelisted(url, parent_url, type);
}

void AdblockPluginAPI::ToggleEnabled(const std::string& url, bool enabled) {
  adblock_->ToggleEnabled(url, enabled);
}

std::string AdblockPluginAPI::GenerateCSSContent() {
  return adblock_->GenerateCSSContent();
}

bool AdblockPluginAPI::block_ads() { return adblock_->block_ads(); }

bool AdblockPluginAPI::block_malware() { return adblock_->block_malware(); }

bool AdblockPluginAPI::dont_track_me() { return adblock_->dont_track_me(); }

void AdblockPluginAPI::Report(const std::string& type,
                              const std::string& documentUrl,
                              const std::string& url, const std::string& rule) {
  return adblock_->Report(type, documentUrl, url, rule);
}
