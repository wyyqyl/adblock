
var API = (function() {
  var filterClasses = require("filterClasses");
  var Filter = filterClasses.Filter;
  var FilterType = filterClasses.FilterType;
  var Subscription = require("subscriptionClasses").Subscription;
  var SpecialSubscription = require("subscriptionClasses").SpecialSubscription;
  var defaultMatcher = require("matcher").defaultMatcher;
  var ElemHide = require("elemHide").ElemHide;
  var Synchronizer = require("synchronizer").Synchronizer;
  var Prefs = require("prefs").Prefs;

  function _matches(url, contentType, documentHost, thirdParty) {
    var filter = defaultMatcher.matchesAny(url, contentType, documentHost, thirdParty);
    if (filter === null) {
      filter = {};
      filter.type = FilterType.NO_MATCH;
    }
    return filter;
  }

  function _isWhitelisted(url, parentUrl, type) {
    // Ignore fragment identifier
    var index = url.indexOf("#");
    if (index >= 0)
      url = url.substring(0, index);

    var filter = _matches(url, type || "DOCUMENT", extractHostFromURL(parentUrl || url), false);
    return filter.type == FilterType.WHITELIST_FILTER ? filter : null;
  }

  return {

    checkFilterMatch: function(url, contentType, documentUrl) {
      var requestHost = extractHostFromURL(url);
      var documentHost = extractHostFromURL(documentUrl);
      var thirdParty = isThirdParty(requestHost, documentHost);
      var filter = _matches(url, contentType, documentHost, thirdParty);
      if (filter.type == FilterType.BLOCKING_FILTER) {
        trigger("BlockingHit", Date.now().toString(), documentUrl, url, filter.text);
      }
      return JSON.stringify(filter);
    },

    getElementHidingSelectors: function(url) {
      var host = extractHostFromURL(url);
      var hostDomain = getBaseDomain(host);
      var selectors = ElemHide.getSelectorsForDomain(host, false);
      return '{"host": "' + host + '", "hostDomain": "' + hostDomain + '", "selectors": ' + JSON.stringify(selectors) + '}';
    },

    isWhitelisted: function(url, parentUrl, type) {
      return _isWhitelisted(url, parentUrl, type) != null;
    },

    toggleEnabled: function(url, enabled) {
      if (enabled) {  // Block ads on this site
        var filter = _isWhitelisted(url);
        while (filter) {
          Subscription.removeFilter(filter);
          if (filter.subscriptions.length) {
            filter.disabled = true;
          }
          filter = _isWhitelisted(url);
        }
      } else {  // Don't block ads on this site
        var host = extractHostFromURL(url).replace(/^www\./, "");
        var filter = Filter.fromText("@@||" + host + "^$document");
        if (!(filter.subscriptions.length && filter.disabled)) {
          Subscription.addFilter(filter);
        }
        filter.disabled = false;
      }
    },

  }

})();
