require.scopes['prefs'] = (function(){
  var exports = {};
  var defaults = {
    __proto__: null,

    locale: "en-US",
    db_directory: null,
    db_file: "adblock.db",
    subscriptions_autoupdate: true,
  };
  var values = Object.create(defaults);
  var path = fileSystem.resolve("prefs.json");
  var listeners = [];
  var isDirty = false;
  var isSaving = false;

  function defineProperty(key) {
    Prefs.__defineGetter__(key, function() {
      return values[key];
    });
    Prefs.__defineSetter__(key, function(value) {
      if (typeof value != typeof defaults[key]) {
        throw new Error("Attempt to change preference type");
      }
      if (value == defaults[key]) {
        delete values[key];
      } else {
        values[key] = value;
      }
      save();
      for (var idx = 0; idx < listeners.length; ++idx) {
        var listener = listeners[idx];
        listener(key);
      }
    })
  }

  function load() {
    fileSystem.read(path, function(result) {
      if (!result.error) {
        try {
          var data = JSON.parse(result.content);
          for (var key in data) {
            if (key in defaults) {
              values[key] = data[key];
            }
          }
        } catch (e) {
          reportError(e);
        }
      }
    });
  }

  function save() {
    if (isSaving) {
      isDirty = true;
      return;
    }
    isDirty = false;
    isSaving = true;
    fileSystem.write(path, JSON.stringify(values), function(result) {
      isSaving = false;
      if (isDirty) {
        save();
      }
    });
  }

  var Prefs = exports.Prefs = {
    addListener: function(listener) {
      if (listeners.indexOf(listener) < 0) {
        listeners.push(listener);
      }
    },
    removeListener: function(listener) {
      var index = listeners.indexOf(listener);
      if (index >= 0) {
        listeners.splice(index, 1);
      }
    }
  };

  for (var key in defaults) {
    defineProperty(key);
  }
  load();

  return exports;
})();
