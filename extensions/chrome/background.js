
chrome.extension.onRequest.addListener(onRequest);

// Special-case domains for which we cannot use style-based hiding rules.
// See http://crbug.com/68705.
var noStyleRulesHosts = ["mail.google.com", "mail.yahoo.com", "www.google.com"];

function onRequest(request, sender, sendResponse) {
  switch (request.reqtype) {
    case "get-settings":
      var hostDomain = null;
      var selectors = null;

      var tabId = -1;
      var frameId = -1;
      if (sender.tab) {
        tabId = sender.tab.id;
        frameId = getFrameId(tabId, request.frameUrl);
      }

      var enabled = !isFrameWhitelisted(tabId, frameId, "DOCUMENT") && !isFrameWhitelisted(tabId, frameId, "ELEMHIDE");
      if (enabled && request.selectors) {
        var noStyleRules = false;
        var hidding = API.getElementHidingSelectors(request.frameUrl);
        var host = hidding.host;
        hostDomain = hidding.hostDomain;
        for (var i = 0; i < noStyleRulesHosts.length; i++) {
          var noStyleHost = noStyleRulesHosts[i];
          if (host == noStyleHost || (host.length > noStyleHost.length &&
                                      host.substr(host.length - noStyleHost.length - 1) == "." + noStyleHost))
          {
            noStyleRules = true;
          }
        }
        selectors = hidding.selectors;
        if (noStyleRules) {
          selectors = selectors.filter(function(s) {
            return !/\[style[\^\$]?=/.test(s);
          });
        }
      }

      sendResponse({enabled: enabled, hostDomain: hostDomain, selectors: selectors});
      break;
    case "should-collapse":
      var tabId = -1;
      var frameId = -1;
      if (sender.tab) {
        tabId = sender.tab.id;
        frameId = getFrameId(tabId, request.documentUrl);
      }

      if (isFrameWhitelisted(tabId, frameId, "DOCUMENT")) {
        sendResponse(false);
        break;
      }

      var filter = API.checkFilterMatch(request.url, request.type, request.documentUrl);
      if (filter.type == FilterType.BLOCKING_FILTER) {
        sendResponse(filter.collapse === null ? true : filter.collapse);
      } else {
        sendResponse(false);
      }
      break;
    default:
      sendResponse({});
      break;
  }
}

/**
 * This function is a hack - we only know the tabId and document URL for a
 * message but we need to know the frame ID. Try to find it in webRequest's
 * frame data.
 */
function getFrameId(tabId, url) {
  if (tabId in frames) {
    for (var f in frames[tabId]) {
      if (getFrameUrl(tabId, f) == url)
        return f;
    }
  }
  return -1;
}

function IsAdblockPluginInstalled() {
  var mimetype = navigator.mimeTypes["application/x-adblock"];
  if (typeof(mimetype) == "undefined") {
    return false;
  }
  return true;
}

function isWhitelisted(url, parentUrl, type)
{
  // Ignore fragment identifier
  var index = url.indexOf("#");
  if (index >= 0)
    url = url.substring(0, index);

  var filter = API.checkFilterMatch(url, type || "DOCUMENT", parentUrl || url);
  return filter.type == FilterType.WHITELIST_FILTER ? true : false;
}
