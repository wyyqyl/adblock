
chrome.tabs.onUpdated.addListener(function(tabId, changeInfo, tab) {
  if(changeInfo.status == "loading")
    refreshIcon(tab);
});
chrome.tabs.onActivated.addListener(function(activeInfo) {
  chrome.tabs.get(activeInfo.tabId, refreshIcon);
});
chrome.windows.onFocusChanged.addListener(function(windowId) {
  chrome.tabs.query({active: true, windowId: windowId}, function(tabs) {
    tabs.forEach(refreshIcon);
  });
});

function refreshIcon(tab) {
  // The tab could have been closed by the time this function is called
  if(!tab)
    return;

  var excluded = API.isWhitelisted(tab.url);
  var iconFilename = excluded ? "icons/asd-19-whitelisted.png" : "icons/asd-19.png";
  
  chrome.pageAction.setIcon({tabId: tab.id, path: iconFilename});

  // Only show icon for pages we can influence (http: and https:)
  if(/^https?:/.test(tab.url))
  {
    chrome.pageAction.setTitle({tabId: tab.id, title: "Anvisoft Adblock"});
    chrome.pageAction.show(tab.id);
  }
}