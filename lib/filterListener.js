/**
 * @fileOverview Component synchronizing filter storage with Matcher instances and ElemHide.
 */

require.scopes['filterListener'] = (function() {
  var exports = {};
  var FilterStorage = require("filterStorage").FilterStorage;
  var FilterNotifier = require("filterNotifier").FilterNotifier;
  var ElemHide = require("elemHide").ElemHide;
  var defaultMatcher = require("matcher").defaultMatcher;
  var filterClasses = require("filterClasses");
  var ActiveFilter = filterClasses.ActiveFilter;
  var RegExpFilter = filterClasses.RegExpFilter;
  var ElemHideBase = filterClasses.ElemHideBase;
  var Prefs = require("prefs").Prefs;

  /**
   * Increases on filter changes, filters will be saved if it exceeds 1.
   * @type Integer
   */
  var isDirty = 0;

  /**
   * This object can be used to change properties of the filter change listeners.
   * @class
   */
  var FilterListener = exports.FilterListener = {

    /**
     * Increases "dirty factor" of the filters and calls FilterStorage.saveToDisk()
     * if it becomes 1 or more. Save is executed delayed to prevent multiple
     * subsequent calls. If the parameter is 0 it forces saving filters if any
     * changes were recorded after the previous save.
     */
    setDirty: function(/**Integer*/ factor) {
      if (factor == 0 && isDirty > 0)
        isDirty = 1;
      else
        isDirty += factor;
      if (isDirty >= 1)
        FilterStorage.saveToDisk();
    },

    /**
     * Initializes filter listener on startup, registers the necessary hooks.
     */
    init: function() {
      FilterNotifier.addListener(function(action, item, newValue, oldValue) {
        var match = /^(\w+)\.(.*)/.exec(action);
        if (match && match[1] == "filter")
          onFilterChange(match[2], item, newValue, oldValue);
        else if (match && match[1] == "subscription")
          onSubscriptionChange(match[2], item, newValue, oldValue);
        else
          onGenericChange(action, item);
      });

      FilterStorage.loadFromDisk();
    }
  };


  /**
   * Notifies Matcher instances or ElemHide object about a new filter
   * if necessary.
   * @param {Filter} filter filter that has been added
   */
  function addFilter(filter) {
    if (!(filter instanceof ActiveFilter) || filter.disabled)
      return;

    var hasEnabled = false;
    for (var i = 0; i < filter.subscriptions.length; i++)
      if (!filter.subscriptions[i].disabled)
        hasEnabled = true;
    if (!hasEnabled)
      return;

    if (filter instanceof RegExpFilter)
      defaultMatcher.add(filter);
    else if (filter instanceof ElemHideBase)
      ElemHide.add(filter);
  }

  /**
   * Notifies Matcher instances or ElemHide object about removal of a filter
   * if necessary.
   * @param {Filter} filter filter that has been removed
   */
  function removeFilter(filter) {
    if (!(filter instanceof ActiveFilter))
      return;

    if (!filter.disabled) {
      var hasEnabled = false;
      for (var i = 0; i < filter.subscriptions.length; i++)
        if (!filter.subscriptions[i].disabled)
          hasEnabled = true;
      if (hasEnabled)
        return;
    }

    if (filter instanceof RegExpFilter)
      defaultMatcher.remove(filter);
    else if (filter instanceof ElemHideBase)
      ElemHide.remove(filter);
  }

  /**
   * Subscription change listener
   */
  function onSubscriptionChange(action, subscription, newValue, oldValue) {
    FilterListener.setDirty(1);

    if (action != "added" && action != "removed" && action != "disabled" && action != "updated")
      return;

    if (action != "removed" && !(subscription.url in FilterStorage.knownSubscriptions)) {
      // Ignore updates for subscriptions not in the list
      return;
    }

    if ((action == "added" || action == "removed" || action == "updated") && subscription.disabled) {
      // Ignore adding/removing/updating of disabled subscriptions
      return;
    }

    if (action == "added" || action == "removed" || action == "disabled") {
      var method = (action == "added" || (action == "disabled" && newValue == false) ? addFilter : removeFilter);
      if (subscription.filters)
        subscription.filters.forEach(method);
    } else if (action == "updated") {
      subscription.oldFilters.forEach(removeFilter);
      subscription.filters.forEach(addFilter);
    }
  }

  /**
   * Filter change listener
   */
  function onFilterChange(action, filter, newValue, oldValue) {
    if (action == "hitCount" || action == "lastHit")
      FilterListener.setDirty(0.002);
    else
      FilterListener.setDirty(1);

    if (action != "added" && action != "removed" && action != "disabled")
      return;

    if ((action == "added" || action == "removed") && filter.disabled) {
      // Ignore adding/removing of disabled filters
      return;
    }

    if (action == "added" || (action == "disabled" && newValue == false))
      addFilter(filter);
    else
      removeFilter(filter);
  }

  /**
   * Generic notification listener
   */
  function onGenericChange(action) {
    if (action == "load") {
      isDirty = 0;

      defaultMatcher.clear();
      ElemHide.clear();
      for (var idx = 0; idx < FilterStorage.subscriptions.length; ++idx) {
        var subscription = FilterStorage.subscriptions[idx]
        if (!subscription.disabled)
          subscription.filters.forEach(addFilter);
      }
    } else if (action == "save") {
      isDirty = 0;
    }
  }

  return exports;
})();

