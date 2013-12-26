#include <string>
#include <sstream>
#include <boost/weak_ptr.hpp>
#include "JSAPIAuto.h"
#include "BrowserHost.h"
#include "AdblockPlugin.h"
#include <adblock.h>

#ifndef H_AdblockPluginAPI
#define H_AdblockPluginAPI

class AdblockPluginAPI : public FB::JSAPIAuto {
 public:
  AdblockPluginAPI(const AdblockPluginPtr& plugin,
                   const FB::BrowserHostPtr& host, adblock::AdBlockPtr adblock)
      : plugin_(plugin), host_(host), adblock_(adblock) {
    registerMethod("checkFilterMatch",
                   make_method(this, &AdblockPluginAPI::CheckFilterMatch));
    registerMethod(
        "getElementHidingSelectors",
        make_method(this, &AdblockPluginAPI::GetElementHidingSelectors));
    registerMethod("isWhitelisted",
                   make_method(this, &AdblockPluginAPI::IsWhitelisted));
    registerMethod("toggleEnabled",
                   make_method(this, &AdblockPluginAPI::ToggleEnabled));
    registerMethod("generateCSSContent",
                   make_method(this, &AdblockPluginAPI::GenerateCSSContent));
    registerMethod("blockAds", make_method(this, &AdblockPluginAPI::block_ads));
    registerMethod("blockMalware",
                   make_method(this, &AdblockPluginAPI::block_malware));
    registerMethod("dontTrackMe",
                   make_method(this, &AdblockPluginAPI::dont_track_me));
    registerMethod("report", make_method(this, &AdblockPluginAPI::Report));
  }

  virtual ~AdblockPluginAPI() {}

  std::string CheckFilterMatch(const std::string& location,
                               const std::string& type,
                               const std::string& document);

  std::string GetElementHidingSelectors(const std::string& domain);

  bool IsWhitelisted(const std::string& url, const std::string& parent_url,
                     const std::string& type);

  void ToggleEnabled(const std::string& url, bool enabled);

  bool block_ads();
  bool block_malware();
  bool dont_track_me();

  std::string GenerateCSSContent();

  void Report(const std::string& type, const std::string& documentUrl,
              const std::string& url, const std::string& filter);

 private:
  AdblockPluginWeakPtr plugin_;
  FB::BrowserHostPtr host_;

  adblock::AdBlockPtr adblock_;
};

#endif  // H_AdblockPluginAPI
