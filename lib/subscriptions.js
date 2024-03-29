require.scopes['subscriptions'] = (function() {
  var exports = {};

  exports.NormalSubscriptions = [{
      "specialization": "English",
      "title": "EasyList",
      "url": "http://cloud2.anvisoft.com/interface/adblock/list/easylist.txt",
      "homepage": "https://easylist.adblockplus.org/",
      "author": "fanboy, MonztA, Famlam, Khrin",
      "prefixes": "en",
      "type": "subscription"
  }, {
      "specialization": "Bahasa Indonesia",
      "title": "ABPindo+EasyList",
      "url": "http://cloud2.anvisoft.com/interface/adblock/list/abpindo+easylist.txt",
      "homepage": "http://abpindo.blogspot.com/",
      "author": "heradhis",
      "prefixes": "id",
      "type": "subscription"
  }, {
      "specialization": "\u0431\u044a\u043b\u0433\u0430\u0440\u0441\u043a\u0438",
      "title": "Bulgarian list+EasyList",
      "url": "http://cloud2.anvisoft.com/interface/adblock/list/bulgarian_list+easylist.txt",
      "homepage": "http://stanev.org/abp/",
      "author": "\u0410\u043b\u0435\u043a\u0441\u0430\u043d\u0434\u044a\u0440 \u0421\u0442\u0430\u043d\u0435\u0432",
      "prefixes": "bg",
      "type": "subscription"
  }, {
      "specialization": "\u4e2d\u6587",
      "title": "ChinaList+EasyList",
      "url": "http://cloud2.anvisoft.com/interface/adblock/list/chinalist+easylist.txt",
      "homepage": "https://code.google.com/p/adblock-chinalist/",
      "author": "Gythialy",
      "prefixes": "zh",
      "type": "subscription"
  }, {
      "specialization": "Nederlands",
      "title": "DutchAdblockList+EasyList",
      "url": "http://cloud2.anvisoft.com/interface/adblock/list/dutchadblocklist+easylist.txt",
      "homepage": "https://code.google.com/p/dutchadblockfilters/",
      "author": "Famlam",
      "prefixes": "nl",
      "type": "subscription"
  }, {
      "specialization": "\u010de\u0161tina, sloven\u010dina",
      "title": "EasyList Czech and Slovak+EasyList",
      "url": "http://cloud2.anvisoft.com/interface/adblock/list/easylistczechslovak+easylist.txt",
      "homepage": "https://code.google.com/p/adblock-czechoslovaklist/",
      "author": "tomasko126",
      "prefixes": "cs,sk",
      "type": "subscription"
  }, {
      "specialization": "Deutsch",
      "title": "EasyList Germany+EasyList",
      "url": "http://cloud2.anvisoft.com/interface/adblock/list/easylistgermany+easylist.txt",
      "homepage": "https://easylist.adblockplus.org/",
      "author": "MonztA, Famlam",
      "prefixes": "de",
      "type": "subscription"
  }, {
      "specialization": "italiano",
      "title": "EasyList Italy+EasyList",
      "url": "http://cloud2.anvisoft.com/interface/adblock/list/easylistitaly+easylist.txt",
      "homepage": "https://easylist.adblockplus.org/",
      "author": "Khrin",
      "prefixes": "it",
      "type": "subscription"
  }, {
      "specialization": "latvie\u0161u valoda",
      "title": "Latvian List+EasyList",
      "url": "http://cloud2.anvisoft.com/interface/adblock/list/latvianlist+easylist.txt",
      "homepage": "http://latvian-list.site11.com/",
      "author": "anonymous74100",
      "prefixes": "lv",
      "type": "subscription"
  }, {
      "specialization": "\u0627\u0644\u0639\u0631\u0628\u064a\u0629",
      "title": "Liste AR+Liste FR+EasyList",
      "url": "http://cloud2.anvisoft.com/interface/adblock/list/liste_ar+liste_fr+easylist.txt",
      "homepage": "https://code.google.com/p/liste-ar-adblock/",
      "author": "smed79",
      "prefixes": "ar",
      "type": "subscription"
  }, {
      "specialization": "fran\u00e7ais",
      "title": "Liste FR+EasyList",
      "url": "http://cloud2.anvisoft.com/interface/adblock/list/liste_fr+easylist.txt",
      "homepage": "http://adblock-listefr.com/",
      "author": "Lian, Crits, smed79",
      "prefixes": "fr",
      "type": "subscription"
  }, {
      "specialization": "rom\u00e2nesc",
      "title": "ROList+EasyList",
      "url": "http://cloud2.anvisoft.com/interface/adblock/list/rolist+easylist.txt",
      "homepage": "http://www.zoso.ro/rolist",
      "author": "MenetZ, Zoso",
      "prefixes": "ro",
      "type": "subscription"
  }, {
      "specialization": "\u0440\u0443\u0441\u0441\u043a\u0438\u0439, \u0443\u043a\u0440\u0430\u0457\u043d\u0441\u044c\u043a\u0430",
      "title": "RuAdList+EasyList",
      "url": "http://cloud2.anvisoft.com/interface/adblock/list/ruadlist+easylist.txt",
      "homepage": "https://code.google.com/p/ruadlist/",
      "author": "Lain_13",
      "prefixes": "ru,uk",
      "type": "subscription"
  }, {
      "specialization": "suomi",
      "title": "Wiltteri+EasyList",
      "url": "http://cloud2.anvisoft.com/interface/adblock/list/wiltteri+easylist.txt",
      "homepage": "http://wiltteri.net/",
      "author": "None",
      "prefixes": "fi",
      "type": "subscription"
  }];

  exports.FeatureSubscriptions = [{
      feature: "malware",
      homepage: "http://malwaredomains.com/",
      title: "Malware Domains",
      url: "http://cloud2.anvisoft.com/interface/adblock/list/malwaredomains_full.txt"
  }];

  return exports;
})();