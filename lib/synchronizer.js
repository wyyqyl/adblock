/**
 * @fileOverview Manages synchronization of filter subscriptions.
 */

require.scopes['synchronizer'] = (function() {
  var exports = {};
  var DownloaderObj = require("downloader");
  var Downloader = DownloaderObj.Downloader;
  var Downloadable = DownloaderObj.Downloadable;
  var MILLIS_IN_SECOND = DownloaderObj.MILLIS_IN_SECOND;
  var MILLIS_IN_MINUTE = DownloaderObj.MILLIS_IN_MINUTE;
  var MILLIS_IN_HOUR = DownloaderObj.MILLIS_IN_HOUR;
  var MILLIS_IN_DAY = DownloaderObj.MILLIS_IN_DAY;
  var filterClasses = require("filterClasses");
  var Filter = filterClasses.Filter;
  var CommentFilter = filterClasses.CommentFilter;
  var FilterStorage = require("filterStorage").FilterStorage;
  var FilterNotifier = require("filterNotifier").FilterNotifier;
  var Prefs = require("prefs").Prefs;
  var subscriptionClasses = require("subscriptionClasses");
  var Subscription = subscriptionClasses.Subscription;
  var DownloadableSubscription = subscriptionClasses.DownloadableSubscription;
  var Utils = require("utils").Utils;
  var INITIAL_DELAY = 6 * MILLIS_IN_MINUTE;
  var CHECK_INTERVAL = 1 * MILLIS_IN_HOUR;
  var DEFAULT_EXPIRATION_INTERVAL = 5 * MILLIS_IN_DAY;
  var downloader = null;

  /**
   * The object providing actual downloading functionality.
   * @type Downloader
   */
  var downloader = null;

  /**
   * This object is responsible for downloading filter subscriptions whenever
   * necessary.
   * @class
   */
  var Synchronizer = exports.Synchronizer = {
    /**
     * Called on module startup.
     */
    init: function() {
      downloader = new Downloader(this._getDownloadables.bind(this), INITIAL_DELAY, CHECK_INTERVAL);
      downloader.onExpirationChange = this._onExpirationChange.bind(this);
      downloader.onDownloadStarted = this._onDownloadStarted.bind(this);
      downloader.onDownloadSuccess = this._onDownloadSuccess.bind(this);
      downloader.onDownloadError = this._onDownloadError.bind(this);
    },

    /**
     * Checks whether a subscription is currently being downloaded.
     * @param {String} url  URL of the subscription
     * @return {Boolean}
     */
    isExecuting: function(url) {
      return downloader.isDownloading(url);
    },

    /**
     * Starts the download of a subscription.
     * @param {DownloadableSubscription} subscription  Subscription to be downloaded
     * @param {Boolean} manual  true for a manually started download (should not trigger fallback requests)
     */
    execute: function(subscription, manual) {
      downloader.download(this._getDownloadable(subscription, manual));
    },

    /**
     * Yields Downloadable instances for all subscriptions that can be downloaded.
     */
    _getDownloadables: function() {
      var downloadables = [];
      if (Prefs.subscriptions_autoupdate) {
        for (var idx = 0; idx < FilterStorage.subscriptions.length; ++idx) {
          var subscription = FilterStorage.subscriptions[idx];
          if (subscription instanceof DownloadableSubscription)
            downloadables.push(this._getDownloadable(subscription, false));
        }
      }
      return result;
    },

    /**
     * Creates a Downloadable instance for a subscription.
     * @return {Downloadable}
     */
    _getDownloadable: function(/**Subscription*/ subscription, /**Boolean*/ manual) {
      var result = new Downloadable(subscription.url);
      if (subscription.lastDownload != subscription.lastSuccess)
        result.lastError = subscription.lastDownload * MILLIS_IN_SECOND;
      result.lastCheck = subscription.lastCheck * MILLIS_IN_SECOND;
      result.lastVersion = subscription.version;
      result.softExpiration = subscription.softExpiration * MILLIS_IN_SECOND;
      result.hardExpiration = subscription.expires * MILLIS_IN_SECOND;
      result.manual = manual;
      return result;
    },

    _onExpirationChange: function(downloadable) {
      var subscription = Subscription.fromURL(downloadable.url);
      subscription.lastCheck = Math.round(downloadable.lastCheck / MILLIS_IN_SECOND);
      subscription.softExpiration = Math.round(downloadable.softExpiration / MILLIS_IN_SECOND);
      subscription.expires = Math.round(downloadable.hardExpiration / MILLIS_IN_SECOND);
    },

    _onDownloadStarted: function(downloadable) {
      var subscription = Subscription.fromURL(downloadable.url);
      FilterNotifier.triggerListeners("subscription.downloadStatus", subscription);
    },

    _onDownloadSuccess: function(downloadable, responseText, errorCallback, redirectCallback) {
      var lines = responseText.split(/[\r\n]+/);
      var match = /\[Adblock(?:\s*Plus\s*([\d\.]+)?)?\]/i.exec(lines[0]);
      if (!match)
        return errorCallback("synchronize_invalid_data");
      var minVersion = parseFloat(match[1]);

      // Don't remove parameter comments immediately but add them to a list first,
      // they need to be considered in the checksum calculation.
      var remove = [];
      var params = {
        redirect: null,
        homepage: null,
        title: null,
        version: null,
        expires: null
      };
      for (var i = 1; i < lines.length; i++) {
        var match = /^\s*!\s*(\w+)\s*:\s*(.*)/.exec(lines[i]);
        if (match) {
          var keyword = match[1].toLowerCase();
          var value = match[2];
          if (keyword in params) {
            params[keyword] = value;
            remove.push(i);
          } else if (keyword == "checksum") {
            lines.splice(i--, 1);
            var checksum = Utils.generateChecksum(lines);
            if (checksum && checksum != value.replace(/=+$/, ""))
              return errorCallback("synchronize_checksum_mismatch");
          }
        }
      }

      if (params.redirect)
        return redirectCallback(params.redirect);

      // Handle redirects
      var subscription = Subscription.fromURL(downloadable.redirectURL || downloadable.url);
      if (downloadable.redirectURL && downloadable.redirectURL != downloadable.url) {
        var oldSubscription = Subscription.fromURL(downloadable.url);
        subscription.title = oldSubscription.title;
        subscription.disabled = oldSubscription.disabled;
        subscription.lastCheck = oldSubscription.lastCheck;

        if (oldSubscription.url in FilterStorage.knownSubscriptions) {
          FilterStorage.removeSubscription(oldSubscription);
          FilterStorage.addSubscription(subscription);
        }

        delete Subscription.knownSubscriptions[oldSubscription.url];
      }

      // The download actually succeeded
      subscription.lastSuccess = subscription.lastDownload = Math.round(Date.now() / MILLIS_IN_SECOND);
      subscription.downloadStatus = "synchronize_ok";
      subscription.errors = 0;

      // Remove lines containing parameters
      for (var i = remove.length - 1; i >= 0; i--)
        lines.splice(remove[i], 1);

      // Process parameters
      if (params.homepage) {
        subscription.homepage = params.homepage;
      }

      if (params.title) {
        subscription.title = params.title;
        subscription.fixedTitle = true;
      } else {
        subscription.fixedTitle = false;
      }

      subscription.version = (params.version ? parseInt(params.version, 10) : 0);

      var expirationInterval = DEFAULT_EXPIRATION_INTERVAL;
      if (params.expires) {
        var match = /^(\d+)\s*(h)?/.exec(params.expires);
        if (match) {
          var interval = parseInt(match[1], 10);
          if (match[2])
            expirationInterval = interval * MILLIS_IN_HOUR;
          else
            expirationInterval = interval * MILLIS_IN_DAY;
        }
      }

      var expiration = downloader.processExpirationInterval(expirationInterval);
      subscription.softExpiration = Math.round(expiration[0] / MILLIS_IN_SECOND);
      subscription.expires = Math.round(expiration[1] / MILLIS_IN_SECOND);

      delete subscription.requiredVersion;
      delete subscription.upgradeRequired;
      if (minVersion) {
        var app_version = require("info").app_version;
        subscription.requiredVersion = minVersion;
        if (minVersion > app_version) {
          subscription.upgradeRequired = true;
          trigger("mustUpdate");
        }
      }

      // Process filters
      lines.shift();
      var filters = [];
      for (var idx = 0; idx < lines.length; ++idx) {
        var line = lines[idx];
        line = Filter.normalize(line);
        if (line)
          filters.push(Filter.fromText(line));
      }

      FilterStorage.updateSubscriptionFilters(subscription, filters);

      return undefined;
    },

    _onDownloadError: function(downloadable, downloadURL, error, channelStatus, responseStatus, redirectCallback) {
      var subscription = Subscription.fromURL(downloadable.url);
      subscription.lastDownload = Math.round(Date.now() / MILLIS_IN_SECOND);
      subscription.downloadStatus = error;

      // Request fallback URL if necessary - for automatic updates only
      if (!downloadable.manual) {
        subscription.errors++;
      }
    },
  };

  return exports;
})();
