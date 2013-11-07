var Prefs = require("prefs").Prefs;
var FilterNotifier = require("filterNotifier").FilterNotifier;
var FilterListener = require("filterListener").FilterListener;
var prefsInitDone = false;
var filtersInitDone = false;
var isFirstRun = false;

function checkInitialized() {
  if (prefsInitDone && filtersInitDone) {
    checkInitialized = function() { };
    trigger("init", isFirstRun);
  }
}

Prefs._initListener = function() {
  prefsInitDone = true;
  checkInitialized();
}

FilterNotifier.addListener(function(action) {
  if (action === "load") {

  }
});

function initAdblock() {
  FilterListener.init();
}
