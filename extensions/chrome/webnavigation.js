
chrome.webNavigation.onBeforeNavigate.addListener(onBeforeNavigate);

function onBeforeNavigate(details) {
  if (details.tabId == -1) return;
  filter = API.checkFilterMatch(details.url, "DOCUMENT", details.url);
  if(filter.malware)
    chrome.tabs.update(details.tabId, {url: "http://www.anvisoft.com/"});
}
