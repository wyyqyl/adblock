
chrome.webNavigation.onBeforeNavigate.addListener(onBeforeNavigate);
chrome.webNavigation.onCreatedNavigationTarget.addListener(onCreatedNavigationTarget);
chrome.tabs.onUpdated.addListener(onUpdated);

function onBeforeNavigate(details) {
  if (!API.enabled()) return;
  if (details.tabId == -1) return;
  filter = API.checkFilterMatch(details.url, "DOCUMENT", details.url);
  if(filter.type != FilterType.WHITELIST_FILTER && filter.malware)
    chrome.tabs.update(details.tabId, {url: "http://WANGYAOYAO/adblock/malware.php?url=" + details.url});
}

var tabsLoading = {};
function onCreatedNavigationTarget(details) {
  if (!API.enabled()) return;
  if (isFrameWhitelisted(details.sourceTabId, details.sourceFrameId))
    return;

  var openerUrl = getFrameUrl(details.sourceTabId, details.sourceFrameId);
  if (!openerUrl)
  {
    // We don't know the opener tab
    return;
  }
  tabsLoading[details.tabId] = openerUrl;

  checkPotentialPopup(details.tabId, details.url, openerUrl);
}

function onUpdated(tabId, changeInfo, tab) {
  if (!API.enabled()) return;
  if (!(tabId in tabsLoading))
  {
    // Not a pop-up we've previously seen
    return;
  }

  if ("url" in changeInfo)
    checkPotentialPopup(tabId, tab.url, tabsLoading[tabId]);

  if ("status" in changeInfo && changeInfo.status == "complete" && tab.url != "about:blank")
    delete tabsLoading[tabId];
}

function checkPotentialPopup(tabId, url, opener)
{
  var filter = API.checkFilterMatch(url || "about:blank", "POPUP", opener);
  if (filter.type == FilterType.BLOCKING_FILTER)
    chrome.tabs.remove(tabId);
}
