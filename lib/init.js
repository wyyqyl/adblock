var Prefs = require("prefs").Prefs;
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
