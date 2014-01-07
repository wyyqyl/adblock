var Prefs = require("prefs").Prefs;
var FilterNotifier = require("filterNotifier").FilterNotifier;
var FilterListener = require("filterListener").FilterListener;
var Synchronizer = require("synchronizer").Synchronizer;

function load_listener(action) {
  if (action === "load") {
    FilterNotifier.removeListener(load_listener);
    var subscriptionClasses = require("subscriptionClasses");
    var Subscription = subscriptionClasses.Subscription;
    var DownloadableSubscription = subscriptionClasses.DownloadableSubscription;
    if (Subscription.subscriptions.length == 0) {
      var Utils = require("utils").Utils;
      var subscriptionlist = require("subscriptions");
      var node = Utils.chooseFilterSubscription(subscriptionlist.NormalSubscriptions);
      if (node) {
        var subscription = Subscription.fromURL(node.url);
        subscription.disabled = false;
        subscription.title = node.title;
        subscription.homepage = node.homepage;
        if (subscription instanceof DownloadableSubscription && !subscription.lastDownload) {
          Synchronizer.execute(subscription);
        }
      }
      for (var idx = 0; idx < subscriptionlist.FeatureSubscriptions.length; ++idx) {
        var node = subscriptionlist.FeatureSubscriptions[idx];
        var subscription = Subscription.fromURL(node.url);
        subscription.disabled = false;
        subscription.title = node.title;
        subscription.homepage = node.homepage;
        if (subscription instanceof DownloadableSubscription && !subscription.lastDownload) {
          Synchronizer.execute(subscription);
        }
      }
    } else {
      for (var idx = 0; idx < Subscription.subscriptions.length; ++idx) {
        var subscription = Subscription.subscriptions[idx];
        if (subscription instanceof DownloadableSubscription && subscription.filters.length == 0) {
          Synchronizer.execute(subscription);
        }
      }
    }
    return 0;
  }
  return 1;
}

function initAdblock() {
  FilterNotifier.addListener(load_listener);
  FilterListener.init();
  Synchronizer.init();
}
