
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
      return defaultMatcher.matchesAny(url, contentType, documentHost, thirdParty);
    },

  }

})();
