
chrome.webRequest.onBeforeRequest.addListener(onBeforeRequest, {urls: ["http://*/*", "https://*/*"]}, ["blocking"]);

var frames = {};
function onBeforeRequest(details) {
  if (!API.enabled()) return {};
  if (details.tabId == -1) return {};

  var type = details.type;

  // Assume that the first request belongs to the top frame. Chrome may give the
  // top frame the type "object" instead of "main_frame".
  // https://code.google.com/p/chromium/issues/detail?id=281711
  if (details.frameId == 0 && !(details.tabId in frames) && type == "object")
    type = "main_frame";

  if (type == "main_frame" || type == "sub_frame")
    recordFrame(details.tabId, details.frameId, details.parentFrameId, details.url);

  if (type == "main_frame")
    return {};

  // Type names match Mozilla's with main_frame and sub_frame being the only exceptions.
  if (type == "sub_frame")
    type = "SUBDOCUMENT";
  else
    type = type.toUpperCase();

  var frame = (type != "SUBDOCUMENT" ? details.frameId : details.parentFrameId);
  var filter = checkRequest(type, details.tabId, details.url, frame);
  if (filter && filter.type == FilterType.BLOCKING_FILTER) {
    return {cancel: true};
  }
  return {};
}

function recordFrame(tabId, frameId, parentFrameId, frameUrl) {
  if (!(tabId in frames))
    frames[tabId] = {};
  frames[tabId][frameId] = {url: frameUrl, parent: parentFrameId};
}

function getFrameData(tabId, frameId) {
  if (tabId in frames && frameId in frames[tabId]) {
    return frames[tabId][frameId];
  } else if (frameId > 0 && tabId in frames && 0 in frames[tabId]) {
    // We don't know anything about javascript: or data: frames, use top frame
    return frames[tabId][0];
  }
  return null;
}

function getFrameUrl(tabId, frameId) {
  var frameData = getFrameData(tabId, frameId);
  return (frameData ? frameData.url : null);
}

function checkRequest(type, tabId, url, frameId) {
  if (isFrameWhitelisted(tabId, frameId))
    return null;

  var documentUrl = getFrameUrl(tabId, frameId);

  if (!documentUrl)
    return null;

  return API.checkFilterMatch(url, type, documentUrl);
}
