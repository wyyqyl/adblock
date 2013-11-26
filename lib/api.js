
var API = (function() {
  var FilterType = require("filterClasses").FilterType;
  var Subscription = require("subscriptionClasses").Subscription;
  var SpecialSubscription = require("subscriptionClasses").SpecialSubscription;
  var FilterStorage = require("filterStorage").FilterStorage;
  var defaultMatcher = require("matcher").defaultMatcher;
  var ElemHide = require("elemHide").ElemHide;
  var Synchronizer = require("synchronizer").Synchronizer;
  var Prefs = require("prefs").Prefs;

  return {

    checkFilterMatch: function(url, contentType, documentUrl) {
      var requestHost = extractHostFromURL(url);
      var documentHost = extractHostFromURL(documentUrl);
      var thirdParty = isThirdParty(requestHost, documentHost);
      var filter = defaultMatcher.matchesAny(url, contentType, documentHost, thirdParty);
      if (filter === null) {
        filter = {};
        filter.type = FilterType.NO_MATCH;
      }
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

  }

})();
