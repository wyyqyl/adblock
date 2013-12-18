var tab = null;

window.onload = function() {
  document.getElementById("enabled").addEventListener("click", function(e) {
    chrome.extension.sendRequest({reqtype: "toggleEnabled", tab: tab, enabled: e.target.checked});
  });

  chrome.windows.getCurrent(function(w) {
    chrome.tabs.getSelected(w.id, function(t) {
      tab = t;
      chrome.extension.sendRequest({reqtype: "checkEnabled", url: tab.url}, function(enabled) {
        document.getElementById("enabled").checked = enabled;
      });
    });
  });
};
