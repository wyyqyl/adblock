function t5() {
  console.trace();
}
function t4() {
  console.trace();
  t5();
}
function t3() {
  console.trace();
  t4();
}
function t2() {
  console.trace();
  t3();
}
function t1() {
  console.trace();
  t2();
}

var API = (function() {

  return {

    Test: function() {
      _triggerEvent('init', true);
    },

    TestFS: function() {
      _fileSystem.write('D:\\test.log', 'test message', function(result) {
        console.log('WRITE');
        if (result) {
          console.error(result);
          return;
        }

        console.log('READ');
        _fileSystem.read('D:\\test.log', function(result) {
          if (result.error) {
            console.error(result.error);
            return;
          }
          console.log(result.content);

          console.log('MOVE');
          _fileSystem.move('D:\\test.log', 'D:\\test1.log', function(result) {
            if (result) {
              console.error(result);
              return;
            }

            console.log('STAT');
            _fileSystem.stat('D:\\test1.log', function(result) {
              if (result.error) {
                console.error(result.error);
                return;
              }
              console.log('exists: ' + result.exists);
              console.log('isFile: ' + result.isFile);
              console.log('isDirectory: ' + result.isDirectory);
              console.log('lastModified: ' + result.lastModified);

              console.log('REMOVE');
              _fileSystem.remove('D:\\test1.log', function(result) {
                if (result) {
                  console.error(result);
                }
              });
            });
          });
        });
      });
    },

    TestWebrequest: function() {
      var headers = {};
      var url = 'http://www.163.com/';
      _webRequest.GET(url, headers, function(result) {
        console.log("CURL Status: " + result.status.toString(16));
        console.log("HTTP Status: " + result.responseStatus);
        for (header in result.responseHeaders) {
          console.log(header + ": " + result.responseHeaders[header]);
        }
      });
    },

    TestConsole: function() {
      console.info("information")
      t1();
    }
  };

})();

setTimeout(function(init) {
  trigger('init', init);
}, 3000, true);
