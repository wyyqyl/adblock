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
      fileSystem.read(database, function(result) {
        var err = result.error;
        var lines = result.content.split(/[\r\n]+/);
        var parser = new INIParser();
        for (var i = 0; i < lines.length; ++i) {
          parser.process(lines[i]);
        }
        parser.process(null);

        if (!err && Subscription.subscriptions.length == 0) {
          err = new Error("No data in the database");
        }

        if (!err) {
          this.fileProperties = parser.fileProperties;

          if (parser.userFilters) {
            for (var idx = 0; idx < parser.userFilters.length; ++idx) {
              var filter = Filter.fromText(parser.userFilters[idx]);
              Subscription.addfilter(filter, null, undefined, true);
            }
          }
        } else {
          reportError(err);
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

        for (var j = 0; j < subscription.filters.length; ++j) {
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

      return data.join("\n") + "\n";
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

      var subscriptions = Subscription.subscriptions.filter(function(s) {
        return !(s instanceof ExternalSubscription);
      });
      fileSystem.write(database, this._generateFilterData(subscriptions), function(e) {
        if (e) {
          reportError(e);
        }
        if (!explicitFile) {
          this._saving = false;
          if (this._needsSave) {
            this._needsSave = false;
            this.saveToDisk();
          } else {
            FilterNotifier.triggerListeners("saved");
          }
        }
      }.bind(this));
    }
  };

  function INIParser() {
    this.fileProperties = this.curObj = {};
  }
  INIParser.prototype = {
    wantObj: true,
    fileProperties: null,
    curObj: null,
    curSection: null,
    userFilters: null,

    process: function(val) {
      var match;
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
              Subscription.fromObject(this.curObj);
              break;
            case "subscription filters":
            case "subscription patterns":
              if (Subscription.subscriptions.length) {
                var subscription = Subscription.subscriptions[Subscription.subscriptions.length - 1];
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
    }
  };

  return exports;
})();
