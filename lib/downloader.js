/**
 * @fileOverview Downloads a set of URLs in regular time intervals.
 */
require.scopes['downloader'] = (function() {
  var exports = {};
  var MILLIS_IN_SECOND = exports.MILLIS_IN_SECOND = 1000;
  var MILLIS_IN_MINUTE = exports.MILLIS_IN_MINUTE = 60 * MILLIS_IN_SECOND;
  var MILLIS_IN_HOUR = exports.MILLIS_IN_HOUR = 60 * MILLIS_IN_MINUTE;
  var MILLIS_IN_DAY = exports.MILLIS_IN_DAY = 24 * MILLIS_IN_HOUR;

  /**
   * Creates a new downloader instance.
   * @param {Function} dataSource  Function that will yield downloadable objects on each check
   * @param {Integer} initialDelay  Number of milliseconds to wait before the first check
   * @param {Integer} checkInterval  Interval between the checks
   * @constructor
   */
  var Downloader = exports.Downloader = function Downloader(dataSource, initialDelay, checkInterval) {
    this.dataSource = dataSource;
    this._timer = new Timer();
    this._timer.initWithCallback(function() {
      this._timer.delay = checkInterval;
      this._doCheck();
    }.bind(this), initialDelay);
    this._downloading = Object.create(null);
  };
  Downloader.prototype = {
    /**
     * Timer triggering the downloads.
     * @type nsITimer
     */
    _timer: null,

    /**
     * Map containing the URLs of objects currently being downloaded as its keys.
     */
    _downloading: null,

    /**
     * Function that will yield downloadable objects on each check.
     * @type Function
     */
    dataSource: null,

    /**
     * Maximal time interval that the checks can be left out until the soft
     * expiration interval increases.
     * @type Integer
     */
    maxAbsenseInterval: 1 * MILLIS_IN_DAY,

    /**
     * Minimal time interval before retrying a download after an error.
     * @type Integer
     */
    minRetryInterval: 1 * MILLIS_IN_DAY,

    /**
     * Maximal allowed expiration interval, larger expiration intervals will be
     * corrected.
     * @type Integer
     */
    maxExpirationInterval: 14 * MILLIS_IN_DAY,

    /**
     * Maximal number of redirects before the download is considered as failed.
     * @type Integer
     */
    maxRedirects: 5,

    /**
     * Called whenever expiration intervals for an object need to be adapted.
     * @type Function
     */
    onExpirationChange: null,

    /**
     * Callback to be triggered whenever a download starts.
     * @type Function
     */
    onDownloadStarted: null,

    /**
     * Callback to be triggered whenever a download finishes successfully. The
     * callback can return an error code to indicate that the data is wrong.
     * @type Function
     */
    onDownloadSuccess: null,

    /**
     * Callback to be triggered whenever a download fails.
     * @type Function
     */
    onDownloadError: null,

    /**
     * Checks whether anything needs downloading.
     */
    _doCheck: function() {
      var now = Date.now();
      for (var idx = 0; idx < this.dataSource().length; ++idx) {
        var downloadable = this.dataSource()[idx];
        if (downloadable.lastCheck && now - downloadable.lastCheck > this.maxAbsenseInterval) {
          // No checks for a long time interval - user must have been offline, e.g.
          // during a weekend. Increase soft expiration to prevent load peaks on the
          // server.
          downloadable.softExpiration += now - downloadable.lastCheck;
        }
        downloadable.lastCheck = now;

        // Sanity check: do expiration times make sense? Make sure people changing
        // system clock don't get stuck with outdated subscriptions.
        if (downloadable.hardExpiration - now > this.maxExpirationInterval)
          downloadable.hardExpiration = now + this.maxExpirationInterval;
        if (downloadable.softExpiration - now > this.maxExpirationInterval)
          downloadable.softExpiration = now + this.maxExpirationInterval;

        // Notify the caller about changes to expiration parameters
        if (this.onExpirationChange)
          this.onExpirationChange(downloadable);

        // Does that object need downloading?
        if (downloadable.softExpiration > now && downloadable.hardExpiration > now)
          continue;

        // Do not retry downloads too often
        if (downloadable.lastError && now - downloadable.lastError < this.minRetryInterval)
          continue;

        this._download(downloadable, 0);
      }
    },

    /**
     * Checks whether an address is currently being downloaded.
     */
    isDownloading: function(/**String*/ url) /**Boolean*/ {
      return url in this._downloading;
    },

    /**
     * Starts a download.
     * @param {Downloadable} url  the object to be downloaded
     */
    download: function(downloadable) {
      // Make sure to detach download from the current execution context
      setTimeout(this._download.bind(this, downloadable, 0), 0);
    },

    /**
     * Generates the real download URL for an object by appending various
     * parameters.
     */
    getDownloadUrl: function(/**Downloadable*/ downloadable)  /** String*/ {
      var info = require("info");
      var app = info.app;
      var app_version = info.app_version;
      var url = downloadable.redirectURL || downloadable.url;
      if (url.indexOf("?") >= 0)
        url += "&";
      else
        url += "?";
      url += "app=" + encodeURIComponent(app) +
          "&app_version=" + encodeURIComponent(app_version);
      return url;
    },

    _download: function(downloadable, redirects) {
      if (downloadable.url in this._downloading)
        return;

      var downloadURL = this.getDownloadUrl(downloadable);
      var request = null;

      var errorCallback = function errorCallback(error) {
        var channelStatus = request.resultStatus;
        var responseStatus = request.status;

        reportError("Adblock: Downloading URL " + downloadable.url + " failed (" + error + ")\n" +
                       "Download address: " + downloadURL + "\n" +
                       "Channel status: " + channelStatus + "\n" +
                       "Server response: " + responseStatus);

        if (this.onDownloadError) {
          // Allow one extra redirect if the error handler gives us a redirect URL
          var redirectCallback = null;
          if (redirects <= this.maxRedirects) {
            redirectCallback = function redirectCallback(url) {
              downloadable.redirectURL = url;
              this._download(downloadable, redirects + 1);
            }.bind(this);
          }
          this.onDownloadError(downloadable, downloadURL, error, channelStatus, responseStatus, redirectCallback);
        }
      }.bind(this);

      try {
        request = new XMLHttpRequest();
        request.open("GET", downloadURL);
      } catch (e) {
        errorCallback("synchronize_invalid_url");
        return;
      }

      request.addEventListener("error", function(event) {
        delete this._downloading[downloadable.url];
        errorCallback("synchronize_connection_error");
      }.bind(this), false);

      request.addEventListener("load", function(event) {
        delete this._downloading[downloadable.url];

        // Status will be 0 for non-HTTP requests
        if (request.status && request.status != 200) {
          errorCallback("synchronize_connection_error");
          return;
        }

        this.onDownloadSuccess(downloadable, request.responseText, errorCallback, function redirectCallback(url) {
          if (redirects >= this.maxRedirects) {
            errorCallback("synchronize_connection_error");
          } else {
            downloadable.redirectURL = url;
            this._download(downloadable, redirects + 1);
          }
        }.bind(this));
      }.bind(this), false);

      this._downloading[downloadable.url] = true;
      if (this.onDownloadStarted)
        this.onDownloadStarted(downloadable);
      request.send(null);
    },

    /**
     * Produces a soft and a hard expiration interval for a given supplied
     * expiration interval.
     * @return {Array} soft and hard expiration interval
     */
    processExpirationInterval: function(/**Integer*/ interval) {
      interval = Math.min(Math.max(interval, 0), this.maxExpirationInterval);
      var soft = Math.round(interval * (Math.random() * 0.4 + 0.8));
      var hard = interval * 2;
      var now = Date.now();
      return [now + soft, now + hard];
    }
  };

  /**
   * An object that can be downloaded by the downloadable
   * @param {String} url  URL that has to be requested for the object
   * @constructor
   */
  var Downloadable = exports.Downloadable = function Downloadable(url) {
    this.url = url;
  };
  Downloadable.prototype = {
    /**
     * URL that has to be requested for the object.
     * @type String
     */
    url: null,

    /**
     * URL that the download was redirected to if any.
     * @type String
     */
    redirectURL: null,

    /**
     * Time of last download error or 0 if the last download was successful.
     * @type Integer
     */
    lastError: 0,

    /**
     * Time of last check whether the object needs downloading.
     * @type Integer
     */
    lastCheck: 0,

    /**
     * Object version corresponding to the last successful download.
     * @type Integer
     */
    lastVersion: 0,

    /**
     * Soft expiration interval, will increase if no checks are performed for a
     * while.
     * @type Integer
     */
    softExpiration: 0,

    /**
     * Hard expiration interval, this is fixed.
     * @type Integer
     */
    hardExpiration: 0,
  };

  return exports;
})();
