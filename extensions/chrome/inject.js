var button = document.getElementById("anviadblockforward");
button.addEventListener("click", function() {
  var url = document.getElementById("anviadblockurl").innerText;
  chrome.extension.sendRequest({reqtype: "add2whitelist", url: url});
}, false);
