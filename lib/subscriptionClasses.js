require.scopes['subscriptionClasses'] = (function() {
  var exports = {};
  var filterClasses = require("filterClasses");
  var ActiveFilter = filterClasses.ActiveFilter;
  var BlockingFilter = filterClasses.BlockingFilter;
  var WhitelistFilter = filterClasses.WhitelistFilter;
  var ElemHideBase = filterClasses.ElemHideBase;
  var FilterNotifier = require("filterNotifier").FilterNotifier;

  /**
   * Abstract base class for filter subscriptions
   *
   * @param {String} url    download location of the subscription
   * @param {String} [title]  title of the filter subscription
   * @constructor
   */
  function Subscription(url, title) {
    this.url = url;
    this.filters = [];
    if (title) {
      this._title = title;
    } else {
      this._title = "NewSubscription";
    }
    Subscription.knownSubscriptions[url] = this;
    Subscription.subscriptions.push(this);
  }
  exports.Subscription = Subscription;

  Subscription.prototype = {
    /**
     * Download location of the subscription
     * @type String
     */
    url: null,

    /**
     * Filters contained in the filter subscription
     * @type Array of Filter
     */
    filters: null,

    _title: null,
    _fixedTitle: false,
    _disabled: false,

    /**
     * Title of the filter subscription
     * @type String
     */
    get title() { return this._title; },
    set title(value) {
      if (value != this._title) {
        var oldValue = this._title;
        this._title = value;
        FilterNotifier.triggerListeners("subscription.title", this, value, oldValue);
      }
      return this._title;
    },

    /**
     * Determines whether the title should be editable
     * @type Boolean
     */
    get fixedTitle() { return this._fixedTitle; },
    set fixedTitle(value) {
      if (value != this._fixedTitle) {
        var oldValue = this._fixedTitle;
        this._fixedTitle = value;
        FilterNotifier.triggerListeners("subscription.fixedTitle", this, value, oldValue);
      }
      return this._fixedTitle;
    },

    /**
     * Defines whether the filters in the subscription should be disabled
     * @type Boolean
     */
    get disabled() { return this._disabled; },
    set disabled(value) {
      if (value != this._disabled) {
        var oldValue = this._disabled;
        this._disabled = value;
        FilterNotifier.triggerListeners("subscription.disabled", this, value, oldValue);
      }
      return this._disabled;
    },

    /**
     * Serializes the filter to an array of strings for writing out on the disk.
     * @param {Array of String} buffer  buffer to push the serialization results into
     */
    serialize: function(buffer) {
      buffer.push("[Subscription]");
      buffer.push("url=" + this.url);
      buffer.push("title=" + this._title);
      if (this._fixedTitle) {
        buffer.push("fixedTitle=true");
      }
      if (this._disabled) {
        buffer.push("disabled=true");
      }
    },
    serializeFilters: function(buffer) {
      for (var idx = 0; idx < this.filters.length; ++idx) {
        var filter = this.filters[idx];
        buffer.push(filter.text.replace(/\[/g, "\\["));
      }
    },
    toString: function() {
      var buffer = [];
      this.serialize(buffer);
      return buffer.join("\n");
    }
  };


  /**
   * Joins subscription's filters to the subscription without any notifications.
   * @param {Subscription} subscription filter subscription that should be connected to its filters
   */
  function addSubscriptionFilters(subscription) {
    if (!(subscription.url in Subscription.knownSubscriptions))
      return;

    for (var idx = 0; idx < subscription.filters.length; ++idx) {
      var filter = subscription.filters[idx];
      filter.subscriptions.push(subscription);
    }
  }

  /**
   * Removes subscription's filters from the subscription without any notifications.
   * @param {Subscription} subscription filter subscription to be removed
   */
  function removeSubscriptionFilters(subscription) {
    if (!(subscription.url in Subscription.knownSubscriptions)) {
      return;
    }
    for (var idx = 0; idx < subscription.filters.length; ++idx) {
      var filter = subscription.filters[idx];
      var i = filter.subscriptions.indexOf(subscription);
      if (i >= 0) {
        filter.subscriptions.splice(i, 1);
      }
    }
  }

  /**
   * Finds the filter group that a filter should be added to by default. Will
   * return null if this group doesn't exist yet.
   */
  function getGroupForFilter(filter) {
    var generalSubscription = null;
    for (var idx = 0; idx < Subscription.subscriptions.length; ++idx) {
      var subscription = Subscription.subscriptions[idx];
      if (subscription instanceof SpecialSubscription && !subscription.disabled) {
        if (subscription.isDefaultFor(filter)) {
          return subscription;
        }
        if (!generalSubscription && (!subscription.defaults || !subscription.defaults.length)) {
          generalSubscription = subscription;
        }
      }
    }
    return generalSubscription;
  };

  /**
   * Cache for known filter subscriptions, maps URL to subscription objects.
   * @type Object
   */
  Subscription.knownSubscriptions = { __proto__: null };

  /**
   * List of filter subscriptions containing all filters
   * @type Array of Subscription
   */
  Subscription.subscriptions = [];

  /**
   * Returns a subscription from its URL, creates a new one if necessary.
   * @param {String} url  URL of the subscription
   * @return {Subscription} subscription or null if the subscription couldn't be created
   */
  Subscription.fromURL = function(url) {
    if (url in Subscription.knownSubscriptions) {
      return Subscription.knownSubscriptions[url];
    }
    if (url.length == 0 || url[0] == "~") {
      return new SpecialSubscription(url);
    }
    return new DownloadableSubscription(url, null);
  };

  /**
   * Deserializes a subscription
   *
   * @param {Object}  obj map of serialized properties and their values
   * @return {Subscription} subscription or null if the subscription couldn't be created
   */
  Subscription.fromObject = function(obj) {
    var result;
    if (obj.url.length && obj.url[0] != "~") {
      result = new DownloadableSubscription(obj.url, obj.title);
      if ("downloadStatus" in obj)
        result._downloadStatus = obj.downloadStatus;
      if ("lastSuccess" in obj)
        result.lastSuccess = parseInt(obj.lastSuccess) || 0;
      if ("lastCheck" in obj)
        result._lastCheck = parseInt(obj.lastCheck) || 0;
      if ("expires" in obj)
        result.expires = parseInt(obj.expires) || 0;
      if ("softExpiration" in obj)
        result.softExpiration = parseInt(obj.softExpiration) || 0;
      if ("errors" in obj)
        result._errors = parseInt(obj.errors) || 0;
      if ("version" in obj)
        result.version = parseInt(obj.version) || 0;
      if ("requiredVersion" in obj) {
        var app_version = require("info").app_version;
        result.requiredVersion = parseFloat(obj.requiredVersion);
        if (result.requiredVersion > app_version) {
          result.upgradeRequired = true;
          trigger("mustUpdate");
        }
      }
      if ("homepage" in obj)
        result._homepage = obj.homepage;
      if ("lastDownload" in obj)
        result._lastDownload = parseInt(obj.lastDownload) || 0;
    } else {
      if (!("title" in obj)) {
        if (obj.url == "~wl~") {
          obj.defaults = "whitelist";
        } else if (obj.url == "~fl~") {
          obj.defaults = "blocking";
        } else if (obj.url == "~eh~") {
          obj.defaults = "elemhide";
        }
        if ("defaults" in obj) {
          obj.title = obj.defaults + "Group_title";
        }
      }
      result = new SpecialSubscription(obj.url, obj.title);
      if ("defaults" in obj) {
        result.defaults = obj.defaults.split(" ");
      }
    }
    if ("fixedTitle" in obj) {
      result._fixedTitle = obj.fixedTitle == "true";
    }
    if ("disabled" in obj) {
      result._disabled = obj.disabled == "true";
    }
    return result;
  };

  /**
   * Replaces the list of filters in a subscription by a new list
   * @param {Subscription} subscription filter subscription to be updated
   * @param {Array of Filter} filters new filter lsit
   */
  Subscription.updateSubscriptionFilters = function(subscription, filters) {
    removeSubscriptionFilters(subscription);
    subscription.oldFilters = subscription.filters;
    subscription.filters = filters;
    addSubscriptionFilters(subscription);
    FilterNotifier.triggerListeners("subscription.updated", subscription);
    delete subscription.oldFilters;
  };

  /**
   * Adds a filter subscription to the list
   * @param {Subscription} subscription filter subscription to be added
   * @param {Boolean} silent  if true, no listeners will be triggered (to be used when filter list is reloaded)
   */
  Subscription.addSubscription = function(url, silent) {
    var subscription = Subscription.fromURL(url);
    if (!subscription.lastDownload)
      Synchronizer.execute(subscription);
    if (!silent)
      FilterNotifier.triggerListeners("subscription.added", subscription);
  };

  /**
   * Removes a filter subscription from the list
   * @param {Subscription} subscription filter subscription to be removed
   * @param {Boolean} silent  if true, no listeners will be triggered (to be used when filter list is reloaded)
   */
  Subscription.removeSubscription = function(url, silent) {
    for (var idx = 0; idx < Subscription.subscriptions.length; ++idx) {
      if (Subscription.subscriptions[idx].url == url) {
        var subscription = Subscription.subscriptions[idx];
        removeSubscriptionFilters(subscription);
        Subscription.subscriptions.splice(idx, 1);
        delete Subscription.knownSubscriptions[url];
        console.log("subscription.filters.length: " + subscription.filters.length);
        if (!silent) {
          FilterNotifier.triggerListeners("subscription.removed", subscription);
        }
        return;
      }
    }
  };

  /**
   * Adds a user-defined filter to the list
   * @param {Filter} filter
   * @param {SpecialSubscription} [subscription] particular group that the filter should be added to
   * @param {Integer} [position] position within the subscription at which the filter should be added
   * @param {Boolean} silent  if true, no listeners will be triggered (to be used when filter list is reloaded)
   */
  Subscription.addFilter = function(filter, subscription, position, silent) {
    if (!subscription) {
      var match = filter.subscriptions.some(function(s) {
        return s instanceof SpecialSubscription && !s.disabled;
      });
      if (match) {
        // already added to special subscription
        return;
      }
      subscription = getGroupForFilter(filter);
    }
    if (!subscription) {
      subscription = SpecialSubscription.createForFilter(filter);
      FilterNotifier.triggerListeners("filter.added", filter, subscription, 0);
      return;
    }
    if (typeof position == "undefined") {
      position = subscription.filters.length;
    }
    if (filter.subscriptions.indexOf(subscription) < 0) {
      filter.subscriptions.push(subscription);
    }
    subscription.filters.splice(position, 0, filter);
    if (!silent) {
      FilterNotifier.triggerListeners("filter.added", filter, subscription, position);
    }
  };

  /**
   * Removes a user-defined filter from the list
   * @param {Filter} filter
   * @param {SpecialSubscription} [subscription] a particular filter group that
   *      the filter should be removed from (if ommited will be removed from all subscriptions)
   * @param {Integer} [position]  position inside the filter group at which the
   *      filter should be removed (if ommited all instances will be removed)
   */
  Subscription.removeFilter = function(filter, subscription, position) {
    var subscriptions = subscription ? [subscription] : filter.subscriptions.slice();
    for (var i = 0; i < subscriptions.length; i++) {
      var subscription = subscriptions[i];
      if (subscription instanceof SpecialSubscription) {
        var positions = [];
        if (typeof position == "undefined") {
          var index = -1;
          do {
            index = subscription.filters.indexOf(filter, index + 1);
            if (index >= 0) {
              positions.push(index);
            }
          } while (index >= 0);
        } else {
          positions.push(position);
        }
        for (var j = positions.length - 1; j >= 0; j--) {
          var position = positions[j];
          if (subscription.filters[position] == filter) {
            subscription.filters.splice(position, 1);
            if (subscription.filters.indexOf(filter) < 0) {
              var index = filter.subscriptions.indexOf(subscription);
              if (index >= 0) {
                filter.subscriptions.splice(index, 1);
              }
            }
            FilterNotifier.triggerListeners("filter.removed", filter, subscription, position);
          }
        }
      }
    }
  };

  /**
   * Class for special filter subscriptions (user's filters)
   * @param {String} url see Subscription()
   * @param {String} [title]  see Subscription()
   * @constructor
   * @augments Subscription
   */
  function SpecialSubscription(url, title) {
    Subscription.call(this, url, title);
  }
  exports.SpecialSubscription = SpecialSubscription;

  SpecialSubscription.prototype = {
    __proto__: Subscription.prototype,

    /**
     * Filter types that should be added to this subscription by default
     * (entries should correspond to keys in SpecialSubscription.defaultsMap).
     * @type Array of String
     */
    defaults: null,

    /**
     * Tests whether a filter should be added to this group by default
     * @param {Filter} filter filter to be tested
     * @return {Boolean}
     */
    isDefaultFor: function(filter) {
      if (this.defaults && this.defaults.length) {
        for (var idx = 0; idx < this.defaults.length; ++idx) {
          var type = this.defaults[idx];
          if (filter instanceof SpecialSubscription.defaultsMap[type]) {
            return true;
          }
          if (!(filter instanceof ActiveFilter) && type == "blacklist") {
            return true;
          }
        }
      }
      return false;
    },

    /**
     * See Subscription.serialize()
     */
    serialize: function(buffer) {
      Subscription.prototype.serialize.call(this, buffer);
      if (this.defaults && this.defaults.length) {
        buffer.push("defaults=" + this.defaults.filter(function(type) {
          return type in SpecialSubscription.defaultsMap;
        }).join(" "));
      }
      if (this._lastDownload) {
        buffer.push("lastDownload=" + this._lastDownload);
      }
    }
  };

  SpecialSubscription.defaultsMap = {
    __proto__: null,

    "whitelist": WhitelistFilter,
    "blocking": BlockingFilter,
    "elemhide": ElemHideBase
  };

  /**
   * Creates a new user-defined filter group.
   * @param {String} [title]  title of the new filter group
   * @result {SpecialSubscription}
   */
  SpecialSubscription.create = function(title) {
    var url;
    do {
      url = "~user~" + Math.round(Math.random() * 1000000);
    } while (url in Subscription.knownSubscriptions);
    return new SpecialSubscription(url, title);
  };

  /**
   * Creates a new user-defined filter group and adds the given filter to it.
   * This group will act as the default group for this filter type.
   */
  SpecialSubscription.createForFilter = function(filter) {
    var subscription = SpecialSubscription.create();
    subscription.filters.push(filter);
    for (var type in SpecialSubscription.defaultsMap) {
      if (filter instanceof SpecialSubscription.defaultsMap[type]) {
        subscription.defaults = [type];
      }
    }
    if (!subscription.defaults) {
      subscription.defaults = ["blocking"];
    }
    filter.subscriptions.push(subscription);
    subscription.title = subscription.defaults[0] + "Group_title";
    return subscription;
  };

  /**
   * Abstract base class for regular filter subscriptions (both internally and externally updated)
   * @param {String} url    see Subscription()
   * @param {String} [title]  see Subscription()
   * @constructor
   * @augments Subscription
   */
  function RegularSubscription(url, title) {
    Subscription.call(this, url, title || url);
  }
  exports.RegularSubscription = RegularSubscription;

  RegularSubscription.prototype = {
    __proto__: Subscription.prototype,

    _homepage: null,
    _lastDownload: 0,

    /**
     * Filter subscription homepage if known
     * @type String
     */
    get homepage() { return this._homepage; },
    set homepage(value) {
      if (value != this._homepage) {
        var oldValue = this._homepage;
        this._homepage = value;
        FilterNotifier.triggerListeners("subscription.homepage", this, value, oldValue);
      }
      return this._homepage;
    },

    /**
     * Time of the last subscription download (in seconds since the beginning of the epoch)
     * @type Number
     */
    get lastDownload() { return this._lastDownload; },
    set lastDownload(value) {
      if (value != this._lastDownload) {
        var oldValue = this._lastDownload;
        this._lastDownload = value;
        FilterNotifier.triggerListeners("subscription.lastDownload", this, value, oldValue);
      }
      return this._lastDownload;
    },

    /**
     * See Subscription.serialize()
     */
    serialize: function(buffer) {
      Subscription.prototype.serialize.call(this, buffer);
      if (this._homepage) {
        buffer.push("homepage=" + this._homepage);
      }
      if (this._lastDownload) {
        buffer.push("lastDownload=" + this._lastDownload);
      }
    }
  };

  /**
   * Class for filter subscriptions updated by externally (by other extension)
   * @param {String} url    see Subscription()
   * @param {String} [title]  see Subscription()
   * @constructor
   * @augments RegularSubscription
   */
  function ExternalSubscription(url, title) {
    RegularSubscription.call(this, url, title);
  }
  exports.ExternalSubscription = ExternalSubscription;

  ExternalSubscription.prototype = {
    __proto__: RegularSubscription.prototype,

    /**
     * See Subscription.serialize()
     */
    serialize: function(buffer) {
      throw new Error("Unexpected call, external subscriptions should not be serialized");
    }
  };

  /**
   * Class for filter subscriptions updated by externally (by other extension)
   * @param {String} url  see Subscription()
   * @param {String} [title]  see Subscription()
   * @constructor
   * @augments RegularSubscription
   */
  function DownloadableSubscription(url, title) {
    RegularSubscription.call(this, url, title);
  }
  exports.DownloadableSubscription = DownloadableSubscription;

  DownloadableSubscription.prototype = {
    __proto__: RegularSubscription.prototype,

    _downloadStatus: null,
    _lastCheck: 0,
    _errors: 0,

    /**
     * Status of the last download (ID of a string)
     * @type String
     */
    get downloadStatus() { return this._downloadStatus; },
    set downloadStatus(value) {
      var oldValue = this._downloadStatus;
      this._downloadStatus = value;
      FilterNotifier.triggerListeners("subscription.downloadStatus", this, value, oldValue);
      return this._downloadStatus;
    },

    /**
     * Time of the last successful download (in seconds since the beginning of the
     * epoch).
     */
    lastSuccess: 0,

    /**
     * Time when the subscription was considered for an update last time (in seconds
     * since the beginning of the epoch). This will be used to increase softExpiration
     * if the user doesn't use Adblock Plus for some time.
     * @type Number
     */
    get lastCheck() { return this._lastCheck; },
    set lastCheck(value) {
      if (value != this._lastCheck) {
        var oldValue = this._lastCheck;
        this._lastCheck = value;
        FilterNotifier.triggerListeners("subscription.lastCheck", this, value, oldValue);
      }
      return this._lastCheck;
    },

    /**
     * Hard expiration time of the filter subscription (in seconds since the beginning of the epoch)
     * @type Number
     */
    expires: 0,

    /**
     * Soft expiration time of the filter subscription (in seconds since the beginning of the epoch)
     * @type Number
     */
    softExpiration: 0,

    /**
     * Number of download failures since last success
     * @type Number
     */
    get errors() { return this._errors; },
    set errors(value) {
      if (value != this._errors) {
        var oldValue = this._errors;
        this._errors = value;
        FilterNotifier.triggerListeners("subscription.errors", this, value, oldValue);
      }
      return this._errors;
    },

    /**
     * Version of the subscription data retrieved on last successful download
     * @type Number
     */
    version: 0,

    /**
     * Minimal Adblock Plus version required for this subscription
     * @type Number
     */
    requiredVersion: 0,

    /**
     * Should be true if requiredVersion is higher than current Adblock Plus version
     * @type Boolean
     */
    upgradeRequired: false,

    /**
     * See Subscription.serialize()
     */
    serialize: function(buffer) {
      RegularSubscription.prototype.serialize.call(this, buffer);
      if (this.downloadStatus)
        buffer.push("downloadStatus=" + this.downloadStatus);
      if (this.lastSuccess)
        buffer.push("lastSuccess=" + this.lastSuccess);
      if (this.lastCheck)
        buffer.push("lastCheck=" + this.lastCheck);
      if (this.expires)
        buffer.push("expires=" + this.expires);
      if (this.softExpiration)
        buffer.push("softExpiration=" + this.softExpiration);
      if (this.errors)
        buffer.push("errors=" + this.errors);
      if (this.version)
        buffer.push("version=" + this.version);
      if (this.requiredVersion)
        buffer.push("requiredVersion=" + this.requiredVersion);
    }
  };

  return exports;
})();
