/**
 * @fileOverview Element hiding implementation.
 */

require.scopes['elemHide'] = (function() {
  var exports = {};
  var ElemHideException = require("filterClasses").ElemHideException;

  /**
   * Lookup table, filters by their associated key
   * @type Object
   */
  var filterByKey = { __proto__: null };

  /**
   * Lookup table, keys of the filters by filter text
   * @type Object
   */
  var keyByFilter = { __proto__: null };

  /**
   * Lookup table, keys are known element hiding exceptions
   * @type Object
   */
  var knownExceptions = { __proto__: null };

  /**
   * Lookup table, lists of element hiding exceptions by selector
   * @type Object
   */
  var exceptions = { __proto__: null };

  /**
   * Element hiding component
   * @class
   */
  var ElemHide = exports.ElemHide = {
    /**
     * Removes all known filters
     */
    clear: function() {
      filterByKey = { __proto__: null };
      keyByFilter = { __proto__: null };
      knownExceptions = { __proto__: null };
      exceptions = { __proto__: null };
    },

    /**
     * Add a new element hiding filter
     * @param {ElemHideFilter} filter
     */
    add: function(filter) {
      if (filter instanceof ElemHideException) {
        if (filter.text in knownExceptions)
          return;

        var selector = filter.selector;
        if (!(selector in exceptions))
          exceptions[selector] = [];
        exceptions[selector].push(filter);
        knownExceptions[filter.text] = true;
      } else {
        if (filter.text in keyByFilter)
          return;

        var key;
        do {
          key = Math.random().toFixed(15).substr(5);
        } while (key in filterByKey);

        filterByKey[key] = filter;
        keyByFilter[filter.text] = key;
        ElemHide.isDirty = true;
      }
    },

    /**
     * Removes an element hiding filter
     * @param {ElemHideFilter} filter
     */
    remove: function(filter) {
      if (filter instanceof ElemHideException) {
        if (!(filter.text in knownExceptions))
          return;

        var list = exceptions[filter.selector];
        var index = list.indexOf(filter);
        if (index >= 0)
          list.splice(index, 1);
        delete knownExceptions[filter.text];
      } else {
        if (!(filter.text in keyByFilter))
          return;

        var key = keyByFilter[filter.text];
        delete filterByKey[key];
        delete keyByFilter[filter.text];
      }
    },

    /**
     * Checks whether an exception rule is registered for a filter on a
     * particular domain.
     * @return {ElemHideException}
     */
    getException: function(/**Filter*/ filter, /**String*/ docDomain) {
      var selector = filter.selector;
      if (!(filter.selector in exceptions))
        return null;

      var list = exceptions[filter.selector];
      for (var i = list.length - 1; i >= 0; i--)
        if (list[i].isActiveOnDomain(docDomain))
          return list[i];

      return null;
    },

    generateCSSContent: function () {
      var domains = { __proto__: null };
      var result = [];
      var hasFilters = false;
      for (var key in filterByKey) {
        var filter = filterByKey[key];
        var domain = filter.selectorDomain || "";

        var list;
        if (domain in domains) {
          list = domains[domain];
        } else {
          list = { __proto__: null };
          domains[domain] = list;
        }
        list[filter.selector] = key;
        hasFilters = true;
      }
      if (!hasFilters) {
        return;
      }

      function escapeChar(match) {
        return "\\" + match.charCodeAt(0).toString(16) + " ";
      }

      // Return CSS data
      var cssTemplate = "-moz-binding: url(about:abp-elemhidehit?%ID%#dummy) !important;";
      for (var domain in domains) {
        var rules = [];
        var list = domains[domain];

        if (domain) {
          result.push(("@-moz-document domain(\"" + domain.split(",").join("\"),domain(\"") + "\"){").replace(/[^\x01-\x7F]/g, escapeChar));
        } else {
          result.push("@-moz-document url-prefix(\"http://\"),url-prefix(\"https://\")," + "url-prefix(\"mailbox://\"),url-prefix(\"imap://\")," + "url-prefix(\"news://\"),url-prefix(\"snews://\"){");
        }
        for (var selector in list) {
          result.push(selector.replace(/[^\x01-\x7F]/g, escapeChar) + "{" + cssTemplate.replace("%ID%", list[selector]) + "}");
        }
        result.push("}");
      }
      return result.join("\n") + "\n";
    },

    /**
     * Retrieves an element hiding filter by the corresponding protocol key
     * @return {Filter}
     */
    getFilterByKey: function(/**String*/ key) {
      return (key in filterByKey ? filterByKey[key] : null);
    },

    /**
     * Returns a list of all selectors active on a particular domain (currently
     * used only in Chrome).
     */
    getSelectorsForDomain: function(/**String*/ domain, /**Boolean*/ specificOnly) {
      var result = [];
      for (var key in filterByKey) {
        var filter = filterByKey[key];
        if (specificOnly && (!filter.domains || filter.domains[""]))
          continue;

        if (filter.isActiveOnDomain(domain) && !this.getException(filter, domain))
          result.push(filter.selector);
      }
      return result;
    }
  };

  return exports;
})();
