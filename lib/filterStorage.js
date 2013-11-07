require.scopes['filterStorage'] = (function() {
  var exports = {};
  var Prefs = require("prefs").Prefs;
  var filterClasses = require("filterClasses");
  var Filter = filterClasses.Filter;
  var ActiveFilter = filterClasses.ActiveFilter;
  var subscriptionClasses = require("subscriptionClasses");
  var Subscription = subscriptionClasses.Subscription;
  var SpecialSubscription = subscriptionClasses.SpecialSubscription;
  var ExternalSubscription = subscriptionClasses.ExternalSubscription;
  var FilterNotifier = require("filterNotifier").FilterNotifier;

  /**
   * Version number of the filter storage file format.
   * @type Integer
   */
  var formatVersion = 4;

  /**
   * This class reads user's filters from disk, manages them in memory
   * and writes them back.
   * @class
   */
  var FilterStorage = exports.FilterStorage = {

    get formatVersion() { return formatVersion; },

    /**
     * File that the filter list has been loaded from and should be saved to
     * @type String
     */
    get database() {
      var file = null;
      if (Prefs.db_file) {
        file = fileSystem.resolve(Prefs.db_file);
      }
      if (!file) {
        file = fileSystem.resolve(Prefs.db_directory);
        if (file) {
          file += "adblock.db";
        }
      }
      if (!file) {
        reportError("Adblock: Failed to resolve database location");
      }
      this.__defineGetter__("database", function() { return file; });
      return this.database;
    },

    /**
     * Map of properties listed in the filter storage file before the sections
     * start. Right now this should be only the format version.
     */
    fileProperties: { __proto__: null },

    /**
     * List of filter subscriptions containing all filters
     * @type Array of Subscription
     */
    subscriptions: [],

    /**
     * Map of subscriptions already on the list, by their URL/identifier
     * @type Object
     */
    knownSubscriptions: { __proto__: null },

    /**
     * Finds the filter group that a filter should be added to by default. Will
     * return null if this group doesn't exist yet.
     */
    getGroupForFilter: function(filter) {
      var generalSubscription = null;
      for (var idx = 0; idx < FilterStorage.subscriptions.length; ++idx) {
        var subscription = FilterStorage.subscriptions[idx];
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
    },

    /**
     * Adds a filter subscription to the list
     * @param {Subscription} subscription filter subscription to be added
     * @param {Boolean} silent  if true, no listeners will be triggered (to be used when filter list is reloaded)
     */
    addSubscription: function(subscription, silent) {
      if (subscription.url in FilterStorage.knownSubscriptions) {
        return;
      }
      FilterStorage.subscriptions.push(subscription);
      FilterStorage.knownSubscriptions[subscription.url] = subscription;
      addSubscriptionFilters(subscription);
      if (!silent) {
        FilterNotifier.triggerListeners("subscription.added", subscription);
      }
    },

    /**
     * Removes a filter subscription from the list
     * @param {Subscription} subscription filter subscription to be removed
     * @param {Boolean} silent  if true, no listeners will be triggered (to be used when filter list is reloaded)
     */
    removeSubscription: function(subscription, silent) {
      for (var i = 0; i < FilterStorage.subscriptions.length; i++) {
        if (FilterStorage.subscriptions[i].url == subscription.url) {
          removeSubscriptionFilters(subscription);
          FilterStorage.subscriptions.splice(i--, 1);
          delete FilterStorage.knownSubscriptions[subscription.url];
          if (!silent) {
            FilterNotifier.triggerListeners("subscription.removed", subscription);
          }
          return;
        }
      }
    },

    /**
     * Moves a subscription in the list to a new position.
     * @param {Subscription} subscription filter subscription to be moved
     * @param {Subscription} [insertBefore] filter subscription to insert before
     *        (if omitted the subscription will be put at the end of the list)
     */
    moveSubscription: function(subscription, insertBefore) {
      var currentPos = FilterStorage.subscriptions.indexOf(subscription);
      if (currentPos < 0) {
        return;
      }
      var newPos = insertBefore ? FilterStorage.subscriptions.indexOf(insertBefore) : -1;
      if (newPos < 0) {
        newPos = FilterStorage.subscriptions.length;
      }
      if (currentPos < newPos) {
        newPos--;
      }
      if (currentPos == newPos) {
        return;
      }
      FilterStorage.subscriptions.splice(currentPos, 1);
      FilterStorage.subscriptions.splice(newPos, 0, subscription);
      FilterNotifier.triggerListeners("subscription.moved", subscription);
    },

    /**
     * Replaces the list of filters in a subscription by a new list
     * @param {Subscription} subscription filter subscription to be updated
     * @param {Array of Filter} filters new filter lsit
     */
    updateSubscriptionFilters: function(subscription, filters) {
      removeSubscriptionFilters(subscription);
      subscription.oldFilters = subscription.filters;
      subscription.filters = filters;
      addSubscriptionFilters(subscription);
      FilterNotifier.triggerListeners("subscription.updated", subscription);
      delete subscription.oldFilters;
    },

    /**
     * Adds a user-defined filter to the list
     * @param {Filter} filter
     * @param {SpecialSubscription} [subscription] particular group that the filter should be added to
     * @param {Integer} [position] position within the subscription at which the filter should be added
     * @param {Boolean} silent  if true, no listeners will be triggered (to be used when filter list is reloaded)
     */
    addFilter: function(filter, subscription, position, silent) {
      if (!subscription) {
        if (filter.subscriptions.some(function(s) {
          return s instanceof SpecialSubscription && !s.disabled;
        })) {
          return;
        }
        subscription = FilterStorage.getGroupForFilter(filter);
      }
      if (!subscription) {
        subscription = SpecialSubscription.createForFilter(filter);
        this.addSubscription(subscription);
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
    },

    /**
     * Removes a user-defined filter from the list
     * @param {Filter} filter
     * @param {SpecialSubscription} [subscription] a particular filter group that
     *      the filter should be removed from (if ommited will be removed from all subscriptions)
     * @param {Integer} [position]  position inside the filter group at which the
     *      filter should be removed (if ommited all instances will be removed)
     */
    removeFilter: function(filter, subscription, position) {
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
    },

    /**
     * Moves a user-defined filter to a new position
     * @param {Filter} filter
     * @param {SpecialSubscription} subscription filter group where the filter is located
     * @param {Integer} oldPosition current position of the filter
     * @param {Integer} newPosition new position of the filter
     */
    moveFilter: function(filter, subscription, oldPosition, newPosition) {
      if (!(subscription instanceof SpecialSubscription) || subscription.filters[oldPosition] != filter) {
        return;
      }
      newPosition = Math.min(Math.max(newPosition, 0), subscription.filters.length - 1);
      if (oldPosition == newPosition) {
        return;
      }
      subscription.filters.splice(oldPosition, 1);
      subscription.filters.splice(newPosition, 0, filter);
      FilterNotifier.triggerListeners("filter.moved", filter, subscription, oldPosition, newPosition);
    },

    /**
     * Increases the hit count for a filter by one
     * @param {Filter} filter
     * @param {Window} window  Window that the match originated in (required
     *                         to recognize private browsing mode)
     */
    increaseHitCount: function(filter, wnd) {
      if (!Prefs.savestats || !(filter instanceof ActiveFilter)) {
        return;
      }
      filter.hitCount++;
      filter.lastHit = Date.now();
    },

    /**
     * Resets hit count for some filters
     * @param {Array of Filter} filters  filters to be reset, if null all filters will be reset
     */
    resetHitCounts: function(filters) {
      if (!filters) {
        filters = [];
        for (var text in Filter.knownFilters) {
          filters.push(Filter.knownFilters[text]);
        }
      }
      for (var idx = 0; idx < filters.length; ++idx) {
        var filter = filters[idx];
        filter.hitCount = 0;
        filter.lastHit = 0;
      }
    },

    _loading: false,

    /**
     * Loads all subscriptions from the disk
     * @param {String} [database] File to read from
     */
    loadFromDisk: function(database) {
      if (this._loading)
        return;

      if (!database) {
        database = this.database;
      }

      this._loading = true;
      var parser = new INIParser();
      fileSystem.read(database, function(result) {
        if (result.error) {
          reportError(result.error);
          return;
        }
        var lines = result.content.split(/[\r\n]+/);
        for (var i = 0; i < lines.length; ++i) {
          parser.process(lines[i]);
        }
        parser.process(null);

        // Old special groups might have been converted, remove them if they are empty
        var specialMap = { "~il~": true, "~wl~": true, "~fl~": true, "~eh~": true };
        var knownSubscriptions = { __proto__: null };
        for (var idx = 0; idx < parser.subscriptions.length; ++idx) {
          var subscription = parser.subscriptions[idx];
          if (subscription instanceof SpecialSubscription &&
              subscription.filters.length == 0 &&
              subscription.url in specialMap) {
            parser.subscriptions.splice(idx--, 1);
          } else {
            knownSubscriptions[subscription.url] = subscription;
          }
        }
        this.fileProperties = parser.fileProperties;
        this.subscriptions = parser.subscriptions;
        this.knownSubscriptions = knownSubscriptions;
        Filter.knownFilters = parser.knownFilters;
        Subscription.knownSubscriptions = parser.knownSubscriptions;

        if (parser.userFilters) {
          for (var idx = 0; idx < parser.userFilters.length; ++idx) {
            var filter = Filter.fromText(parser.userFilters[idx]);
            this.addfilter(filter, null, undefined, true);
          }
        }

        this._loading = false;
        FilterNotifier.triggerListeners("load");
        if (database != this.database) {
          this.saveToDisk();
        }
      }.bind(this));
    },

    _generateFilterData: function(subscriptions) {
      var data = [];
      data.push("# Adblock preferences");
      data.push("version="+formatVersion);
      var saved = { __proto__: null };
      var buf = [];
      for (var i = 0; i < subscriptions.length; ++i) {
        var subscription = subscriptions[i];
        for (var j = 0; j < subscriptions.filters.length; ++j) {
          var filter = subscription.filters[j];
          if (!(filter.text in saved)) {
            filter.serialize(buf);
            saved[filter.text] = filter;
            for (var k = 0; k < buf.length; ++k) {
              data.push[buf[k]];
            }
            buf.splice(0);
          }
        }
      }
      for (var i = 0; i < subscriptions.length; ++i) {
        var subscription = subscriptions[i];
        data.push("");
        subscription.serialize(buf);
        if (subscription.filters.length) {
          buf.push("", "[Subscription filters]");
          subscription.serializeFilters(buf);
        }
        for (var j = 0; j < buf.length; ++j) {
          data.push(buf[j]);
        }
        buf.splice(0);
      }

      return data;
    },

    /**
     * Will be set to true if saveToDisk() is running (reentrance protection).
     * Only effetive for default database
     * @type Boolean
     */
    _saving: false,

    /**
     * Will be set to true if a saveToDisk() call arrives while saveToDisk() is
     * already running (delayed execution).
     * @type Boolean
     */
    _needsSave: false,

    /**
     * Saves all subscriptions back to disk
     * @param {String} [database] File to be written
     */
    saveToDisk: function(database) {
      var explicitFile = true;
      if (!database) {
        database = this.database;
        explicitFile = false;
      }
      if (!explicitFile && this._saving) {
        // delayed execution
        this._needsSave = true;
        return;
      }
      if (!explicitFile) {
        this._saving = true;
      }
      fileSystem.write(database, this._generateFilterData(subscriptions), function(e) {
        if (!explicitFile) {
          this._saving = false;
          if (e) {
            reportError(e);
          }
          if (!explicitFile && this._needSave) {
            this._needSave = false;
            this.saveToDisk();
          } else {
            FilterNotifier.triggerListeners("save");
          }
        }
      }.bind(this));
    }
  };

  /**
   * Joins subscription's filters to the subscription without any notifications.
   * @param {Subscription} subscription filter subscription that should be connected to its filters
   */
  function addSubscriptionFilters(subscription) {
    if (!(subscription.url in FilterStorage.knownSubscriptions)) {
      return;
    }
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
    if (!(subscription.url in FilterStorage.knownSubscriptions)) {
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

  function INIParser() {
    this.fileProperties = this.curObj = {};
    this.subscriptions = [];
    this.knownFilters = { __proto__: null };
    this.knownSubscriptions = { __proto__: null };
  }
  INIParser.prototype = {
    subscriptions: null,
    knownFilters: null,
    knownSubscriptions: null,
    wantObj: true,
    fileProperties: null,
    curObj: null,
    curSection: null,
    userFilters: null,

    process: function(val) {
      var origKnownFilters = Filter.knownFilters;
      Filter.knownFilters = this.knownFilters;
      var origKnownSubscriptions = Subscription.knownSubscriptions;
      Subscription.knownSubscriptions = this.knownSubscriptions;
      var match;
      try {
        if (this.wantObj === true && (match = /^(\w+)=(.*)$/.exec(val))) {
          this.curObj[match[1]] = match[2];
        } else if (val === null || (match = /^\s*\[(.+)\]\s*$/.exec(val))) {
          if (this.curObj) {
            // Process current object before going to next section
            switch (this.curSection) {
              case "filter":
              case "pattern":
                if ("text" in this.curObj) {
                  Filter.fromObject(this.curObj);
                }
                break;
              case "subscription":
                var subscription = Subscription.fromObject(this.curObj);
                if (subscription) {
                  this.subscriptions.push(subscription);
                }
                break;
              case "subscription filters":
              case "subscription patterns":
                if (this.subscriptions.length) {
                  var subscription = this.subscriptions[this.subscriptions.length - 1];
                  for (var idx = 0; idx < this.curObj.length; ++idx) {
                    var text = this.curObj[idx];
                    var filter = Filter.fromText(text);
                    subscription.filters.push(filter);
                    filter.subscriptions.push(subscription);
                  }
                }
                break;
              case "user patterns":
                this.userFilters = this.curObj;
                break;
            }
          }
          if (val === null) {
            return;
          }
          this.curSection = match[1].toLowerCase();
          switch (this.curSection) {
            case "filter":
            case "pattern":
            case "subscription":
              this.wantObj = true;
              this.curObj = {};
              break;
            case "subscription filters":
            case "subscription patterns":
            case "user patterns":
              this.wantObj = false;
              this.curObj = [];
              break;
            default:
              this.wantObj = undefined;
              this.curObj = null;
          }
        } else if (this.wantObj === false && val) {
          this.curObj.push(val.replace(/\\\[/g, "["));
        }
      } finally {
        Filter.knownFilters = origKnownFilters;
        Subscription.knownSubscriptions = origKnownSubscriptions;
      }
    }
  };

  return exports;
})();
