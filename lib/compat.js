
function require(module) {
  return require.scopes[module];
}
require.scopes = { __proto__: null };

function reportError(e) {
  console.error(e);
  console.trace();
}

function Timer() { }
Timer.prototype = {
  delay: 0,
  callback: null,
  initWithCallback: function(callback, delay) {
    this.callback = callback;
    this.delay = delay;
    this.scheduleTimeout();
  },
  scheduleTimeout: function() {
    setTimeout(function() {
      try {
        this.callback();
      } catch (e) {
        reportError(e);
      }
      this.scheduleTimeout();
    }.bind(this), this.delay);
  }
};

function XMLHttpRequest() {
  this._requestHeaders = {};
  this._loadHandlers = [];
  this._errorHandlers = [];
};
XMLHttpRequest.prototype = {
  _url: null,
  _requestHeaders: null,
  _responseHeaders: null,
  _loadHandlers: null,
  _errorHandlers: null,
  onload: null,
  onerror: null,
  status: 0,
  readyState: 0,
  responseText: null,

  addEventListener: function(eventName, handler, capture) {
    var list;
    if (eventName == "load")
      list = this._loadHandlers;
    else if (eventName == "error")
      list = this._errorHandlers;
    else
      throw new Error("Event type " + eventName + " not supported");

    if (list.indexOf(handler) < 0)
      list.push(handler);
  },

  removeEventListener: function(eventName, handler, capture) {
    var list;
    if (eventName == "load")
      list = this._loadHandlers;
    else if (eventName == "error")
      list = this._errorHandlers;
    else
      throw new Error("Event type " + eventName + " not supported");

    var index = list.indexOf(handler);
    if (index >= 0)
      list.splice(index, 1);
  },

  open: function(method, url, async, user, password) {
    if (method != "GET")
      throw new Error("Only GET requests are currently supported");
    if (typeof async != "undefined" && !async)
      throw new Error("Sync requests are not supported");
    if (typeof user != "undefined" || typeof password != "undefined")
      throw new Error("User authentification is not supported");
    if (this.readyState != 0)
      throw new Error("Already opened");

    this.readyState = 1;
    this._url = url;
  },

  send: function(data) {
    if (this.readyState != 1)
      throw new Error("XMLHttpRequest.send() is being called before XMLHttpRequest.open()");
    if (typeof data != "undefined" && data)
      throw new Error("Sending data to server is not supported");

    this.readyState = 3;
    webRequest.Get(this._url, this._requestHeaders, function(result) {
      this.resultStatus = result.status;
      this.status = result.responseStatus;
      this.responseText = result.responseText;
      this._responseHeaders = result.responseHeaders;
      this.readyState = 4;

      // Notify event listeners
      var eventName = (this.resultStatus == 0 ? "load" : "error");
      var event = { type: eventName };

      if (this["on" + eventName])
        this.onload.call(this, event);

      var list = this["_" + eventName + "Handlers"];
      for (var i = 0; i < list.length; i++)
        list[i].call(this, event);
    }.bind(this));
  },

  setRequestHeader: function(name, value) {
    if (this.readyState > 1)
      throw new Error("Cannot set request header after sending");

    this._requestHeaders[name] = value;
  },

  getResponseHeader: function(name) {
    name = name.toLowerCase();
    if (!this._responseHeaders || !this._responseHeaders.hasOwnProperty(name))
      return null;
    return this._responseHeaders[name];
  }
};
