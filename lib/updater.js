require.scopes['updater'] = (function() {
  var exports = {};
  var Prefs = require("prefs").Prefs;
  var Info = require("info");
  var DownloaderObj = require("downloader");
  var Downloader = DownloaderObj.Downloader;
  var Downloadable = DownloaderObj.Downloadable;
  var MILLIS_IN_HOUR = DownloaderObj.MILLIS_IN_HOUR;
  var updateUrl = Prefs.update_url.replace(/%NAME%/g, encodeURIComponent(Info.app));
  var callback = null;
  var INITIAL_DELAY = 0.1 * MILLIS_IN_HOUR;
  var CHECK_INTERVAL = 1 * MILLIS_IN_HOUR;
  var EXPIRATION_INTERVAL = 24 * MILLIS_IN_HOUR;
  var TYPE_AUTOMATIC = 0;
  var TYPE_MANUAL = 1;
  var downloader = null;

  function getDownloadable(forceCheck) {
    var url = updateUrl.replace(/%TYPE%/g, forceCheck ? TYPE_MANUAL : TYPE_AUTOMATIC);
    var downloadable = new Downloadable(url);
    downloadable.lastError = Prefs.update_last_error;
    downloadable.lastCheck = Prefs.update_last_check;
    downloadable.softExpiration = Prefs.update_soft_expiration;
    downloadable.hardExpiration = Prefs.update_hard_expiration;
    return downloadable;
  }

  function getDownloadables() {
    var result = [];
    result.push(getDownloadable(false));
    return result;
  }

  function onExpirationChange(downloadable) {
    Prefs.update_last_check = downloadable.lastCheck;
    Prefs.update_soft_expiration = downloadable.softExpiration;
    Prefs.update_hard_expiration = downloadable.hardExpiration;
  }

  function onDownloadSuccess(downloadable, responseText, errorCallback, redirectCallback) {
    Prefs.update_last_error = 0;
    var interval = downloader.processExpirationInterval(EXPIRATION_INTERVAL);
    Prefs.update_soft_expiration = interval[0];
    Prefs.update_hard_expiration = interval[1];
    try {
      var data = JSON.parse(responseText);
      var updateInfo = null;
      if (Info.app in data) {
        updateInfo = data[Info.app];
        if ("app_version" in updateInfo && "url" in updateInfo &&
            parseFloat(updateInfo.app_version) > Info.app_version) {
          trigger("updateAvailable", updateInfo.url);
        }
      }
      if (callback) {
        callback(null);
      }
    } catch (e) {
      reportError(e);
      errorCallback(e);
    }
    callback = null;
  }

  function onDownloadError(downloadable, downloadURL, error, channelStatus, responseStatus, redirectCallback) {
    Prefs.update_last_error = Date.now();
    if (callback) {
      callback(error);
    }
    callback = null;
  }

  var Updater = exports.Updater = {
    init: function() {
      downloader = new Downloader(getDownloadables, INITIAL_DELAY, CHECK_INTERVAL);
      downloader.onExpirationChange = onExpirationChange;
      downloader.onDownloadSuccess = onDownloadSuccess;
      downloader.onDownloadError = onDownloadError;
    },

    checkForUpdates: function(_callback) {
      callback = _callback;
      downloader.download(getDownloadable(true));
    }
  };

  return exports;
})();
