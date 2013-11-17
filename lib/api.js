
var API = (function() {
  var Filter = require("filterClasses").Filter;
  var Subscription = require("subscriptionClasses").Subscription;
  var SpecialSubscription = require("subscriptionClasses").SpecialSubscription;
  var FilterStorage = require("filterStorage").FilterStorage;
  var defaultMatcher = require("matcher").defaultMatcher;
  var ElemHide = require("elemHide").ElemHide;
  var Synchronizer = require("synchronizer").Synchronizer;
  var Prefs = require("prefs").Prefs;
  var checkForUpdates = require("updater").checkForUpdates;

  return {

    checkFilterMatch: function(url, contentType, documentUrl) {
      var requestHost = extractHostFromURL(url);
      var documentHost = extractHostFromURL(documentUrl);
      var thirdParty = isThirdParty(requestHost, documentHost);
      return JSON.stringify(defaultMatcher.matchesAny(url, contentType, documentHost, thirdParty));
    },

    getElementHidingSelectors: function (url) {
      var host = extractHostFromURL(url);
      var hostDomain = getBaseDomain(host);
      var selectors = ElemHide.getSelectorsForDomain(host, false);
      return JSON.stringify('{"host": "' + host + '", "hostDomain": "' + hostDomain + '", "selectors": ' + JSON.stringify(selectors) + '}');
    },

  }

})();
