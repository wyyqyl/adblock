/**
 * @fileOverview This component manages listeners and calls them to distributes
 * messages about filter changes.
 */

require.scopes['filterNotifier'] = (function() {
  var exports = {};

  /**
   * List of registered listeners
   * @type Array of function(action, item, newValue, oldValue)
   */
  var listeners = [];

  /**
   * This class allows registering and triggering listeners for filter events.
   * @class
   */
  var FilterNotifier = exports.FilterNotifier = {

    /**
     * Adds a listener
     */
    addListener: function(/**function(action, item, newValue, oldValue)*/ listener) {
      if (listeners.indexOf(listener) >= 0)
        return;

      listeners.push(listener);
    },

    /**
     * Removes a listener that was previosly added via addListener
     */
    removeListener: function(/**function(action, item, newValue, oldValue)*/ listener) {
      var index = listeners.indexOf(listener);
      if (index >= 0)
        listeners.splice(index, 1);
    },

    /**
     * Notifies listeners about an event
     * @param {String} action event code ("load", "save", "elemhideupdate",
     *                 "subscription.added", "subscription.removed",
     *                 "subscription.disabled", "subscription.title",
     *                 "subscription.lastDownload", "subscription.downloadStatus",
     *                 "subscription.homepage", "subscription.updated",
     *                 "filter.added", "filter.removed", "filter.moved",
     *                 "filter.disabled", "filter.hitCount", "filter.lastHit")
     * @param {Subscription|Filter} item item that the change applies to
     */
    triggerListeners: function(action, item, param1, param2, param3) {
      for (var idx = 0; idx < listeners.length; ++idx) {
        listeners[idx](action, item, param1, param2, param3);
      }
    }
  };

  return exports;
})();
