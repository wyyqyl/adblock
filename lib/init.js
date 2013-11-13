var Prefs = require("prefs").Prefs;
var FilterNotifier = require("filterNotifier").FilterNotifier;
var FilterListener = require("filterListener").FilterListener;
var Synchronizer = require("synchronizer").Synchronizer;
var Updater = require("updater").Updater;
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

function load_listener(action) {
  console.log(action);
  if (action === "load") {
    FilterNotifier.removeListener(load_listener);
    var FilterStorage = require("filterStorage").FilterStorage;
    if (FilterStorage.subscriptions.length == 0) {
      isFirstRun = true;
      var subscriptionClasses = require("subscriptionClasses");
      var Subscription = subscriptionClasses.Subscription;
      var DownloadableSubscription = subscriptionClasses.DownloadableSubscription;
      var Utils = require("utils").Utils;
      var subscriptions = require("subscriptions");
      var node = Utils.chooseFilterSubscription(subscriptions);
      if (node) {
        var subscription = Subscription.fromURL(node.url);
        FilterStorage.addSubscription(subscription);
        subscription.disabled = false;
        subscription.title = node.title;
        subscription.homepage = node.homepage;
        if (subscription instanceof DownloadableSubscription && !subscription.lastDownload) {
          Synchronizer.execute(subscription);
        }
      }
    }
    filtersInitDone = true;
    checkInitialized();
    return 0;
  }
  return 1;
}

function initAdblock() {
  FilterNotifier.addListener(load_listener);
  FilterListener.init();
  Updater.init();
  Synchronizer.init();
}
