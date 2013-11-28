var FilterType = {
  NO_MATCH: 0,
  INVALID_FILTER: 1,
  BLOCKING_FILTER: 2,
  WHITELIST_FILTER: 3,
  ELEMENT_HIDE_FILTER: 4,
  ELEMENT_HIDE_EXCEPTION: 5,
  COMMENT_FILTER: 6
};

var API = (function() {
  var plugin = document.getElementById("AnviAdblockObject");

  return {
    checkFilterMatch: function(url, type, documentUrl) {
      return JSON.parse(plugin.checkFilterMatch(url, type, documentUrl));
    },

    getElementHidingSelectors: function(domain) {
      return JSON.parse(plugin.getElementHidingSelectors(domain));
    },

    isWhitelisted: function(url, parentUrl, type) {
      return plugin.isWhitelisted(url, parentUrl || "", type || "");
    },

    toggleEnabled: function(url, enabled) {
      plugin.toggleEnabled(url, enabled || false);
    }
  };
})();

