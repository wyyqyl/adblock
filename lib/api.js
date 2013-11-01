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

console.log('log');
console.info('info');
console.warn('warn');
console.error('error');
console.trace();
t1();

fileSystem.write('D:\\test.log', 'test message', function(result) {
    console.log('WRITE');
    if (result) {
        console.error(result);
        return;
    }

    console.log('READ');
    fileSystem.read('D:\\test.log', function(result) {
        if (result.error) {
            console.error(result.error);
            return;
        }
        console.log(result.content);

        console.log('MOVE');
        fileSystem.move('D:\\test.log', 'D:\\test1.log', function(result) {
            if (result) {
                console.error(result);
                return;
            }

            console.log('STAT');
            fileSystem.stat('D:\\test1.log', function(result) {
                if (result.error) {
                    console.error(result.error);
                    return;
                }
                console.log('exists: ' + result.exists);
                console.log('isFile: ' + result.isFile);
                console.log('isDirectory: ' + result.isDirectory);
                console.log('lastModified: ' + result.lastWriteTime);

                console.log('REMOVE');
                fileSystem.remove('D:\\test1.log', function(result) {
                    if (result) {
                        console.error(result);
                        return;
                    }
                    console.log(fileSystem.resolve('adblock.lib'));
                });
            });
        });
    });
});

var headers = { };
webRequest.get('http://www.baidu.com', headers, function (result) {
    console.log('status: ' + result.status);
    console.log('response status: ' + result.responseStatus);
    for (header in result.responseHeaders) {
        console.log(header + ': ' + result.responseHeaders[header]);
    }
});

trigger('init', false);
setTimeout(function (p1, p2) {
    console.log('setTimeout');
    console.log(p1);
    console.log(p2);
}, 3000, 'p1', 'p2');
