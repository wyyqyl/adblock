
chrome.webNavigation.onBeforeNavigate.addListener(onBeforeNavigate);

function onBeforeNavigate(details) {
  if (details.tabId == -1) return;
  filter = API.checkFilterMatch(details.url, "DOCUMENT", details.url);
  if(filter.type != FilterType.WHITELIST_FILTER && filter.malware)
    chrome.tabs.update(details.tabId, {url: "http://WANGYAOYAO/adblock/malware.php?url=" + details.url});
}
